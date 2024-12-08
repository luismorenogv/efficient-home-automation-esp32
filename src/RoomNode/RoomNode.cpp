/**
 * @file RoomNode.cpp
 * @brief Coordinates modules in the RoomNode: presence, lights, AC, network.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "RoomNode/RoomNode.h"

RoomNode* RoomNode::instance = nullptr;

RoomNode::RoomNode(uint8_t room_id)
    : room_id(room_id), wifi_channel(0), communications(), presenceSensor(), lights(), airConditioner() {
    instance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
    presenceQueue = xQueueCreate(10, sizeof(uint8_t));
}

// Initializes node: sets up Wi-Fi, ESP-NOW, presence sensor, NTP, and lights schedule
void RoomNode::initialize() {
    Serial.begin(115200);
    delay(1000);

    presenceSensor.initialize();

    communications.initializeWifi();
    if (!communications.initializeESPNOW()) {
        Serial.println("ESP-NOW init failed.");
        tryLater();
    }

    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    WiFi.disconnect(); 

    Serial.println("Joining network...");
    if(!joinNetwork()){
        Serial.println("No master connection.");
        tryLater();
    } else {
        Serial.println("Joined network.");
    }

    // Set initial mode based on current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    uint16_t current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // Initialize lights state and mode
    if(!lights.initializeState(LDR_PIN) || !lights.initializeMode(current_minutes)){
        Serial.println("Lights init failed.");
        tryLater();
    }

    // Set default warm/cold schedules
    Time warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};
    Time cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
    lights.setSchedule(warm, cold);
}

// Attempts to join master network by cycling through channels and waiting for ACK
bool RoomNode::joinNetwork() {
    JoinRoomMsg msg;
    msg.room_id = room_id;
    msg.cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
    msg.warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};

    for (uint8_t i = 0; i < MAX_WIFI_CHANNEL; i++) {
        wifi_channel = (wifi_channel + i) % MAX_WIFI_CHANNEL;
        esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
        communications.registerPeer((uint8_t*)master_mac_addr, wifi_channel);

        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            communications.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (communications.waitForAck(MessageType::JOIN_ROOM, ACK_TIMEOUT_MS)) {
                ack_received = true;
            } else {
                retries++;
                Serial.printf("No ACK, retry (%u/%u)\r\n", retries, MAX_RETRIES);
            }
        }

        if (ack_received) {
            Serial.printf("Master on channel %u\r\n", wifi_channel);
            return true;
        }
        Serial.printf("No ACK in channel %u\r\n", wifi_channel);
        communications.unregisterPeer((uint8_t*)master_mac_addr);
    }
    return false;
}

// Creates FreeRTOS tasks for handling communications, lights, presence, and NTP sync
void RoomNode::run() {
    BaseType_t result;

    result = xTaskCreate(espnowTask, "ESP-NOW Task", 4096, this, 2, &espnowTaskHandle);
    if (result != pdPASS) Serial.println("Failed ESP-NOW Task");

    result = xTaskCreate(lightsControlTask, "Lights Control Task", 4096, this, 2, &lightsControlTaskHandle);
    if (result != pdPASS) Serial.println("Failed Lights Task");

    result = xTaskCreate(presenceTask, "Presence Task", 2048, this, 2, &presenceTaskHandle);
    if (result != pdPASS) Serial.println("Failed Presence Task");

    result = xTaskCreate(NTPSyncTask, "NTPSync Task", 2048, this, 2, &NTPSyncTaskHandle);
    if (result != pdPASS) Serial.println("Failed NTPSync Task");

    Serial.println("RoomNode running tasks...");
}

// Handles incoming ESP-NOW messages from master
void RoomNode::espnowTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    IncomingMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            MessageType msg_type = static_cast<MessageType>(msg.data[0]);
            switch (msg_type) {
                case MessageType::ACK:
                    {
                        if (msg.len != sizeof(AckMsg)) {
                            Serial.println("Invalid ACK length.");
                            break;
                        }
                        AckMsg* payload = reinterpret_cast<AckMsg*>(msg.data);
                        self->communications.ackReceived(msg.mac_addr, payload->acked_msg);
                    }
                    break;
                case MessageType::NEW_SCHEDULE:
                    {
                        if (msg.len != sizeof(NewScheduleMsg)) {
                            Serial.println("Invalid NEW_SCHEDULE length.");
                            break;
                        }
                        NewScheduleMsg* payload = reinterpret_cast<NewScheduleMsg*>(msg.data);
                        self->lights.setSchedule(payload->warm, payload->cold);
                        self->communications.sendAck(master_mac_addr, MessageType::NEW_SCHEDULE);
                    }
                    break;
                default:
                    Serial.printf("Unknown message type: %d\r\n", msg_type);
            }
        }
    }
}

// Periodically checks and updates lights mode/brightness
void RoomNode::lightsControlTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);

    while (true) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        uint16_t current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

        self->lights.checkAndUpdateMode(current_minutes);
        self->lights.adjustBrightness(LDR_PIN);

        vTaskDelay(pdMS_TO_TICKS(LIGHTS_CONTROL_PERIOD));
    }
}

// Monitors presence events from LD2410 and controls lights/AC accordingly
void RoomNode::presenceTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    self->presenceSensor.setQueue(self->presenceQueue);
    self->presenceSensor.start();

    bool previous_presence = false;
    while (true) {
        uint8_t presenceState;
        if (xQueueReceive(self->presenceQueue, &presenceState, portMAX_DELAY) == pdTRUE) {
            bool new_presence = (presenceState == HIGH);
            if (new_presence && !previous_presence) {
                Serial.println("Presence detected: Lights ON");
                if(!self->lights.sendCommand(Command::ON)){
                    Serial.println("Lights ON failed");
                }
            } else if (!new_presence && previous_presence) {
                Serial.println("No presence: OFF AC/Lights");
                self->airConditioner.turnOff();
                if(self->lights.isOn()){
                    if(!self->lights.sendCommand(Command::OFF)){
                        Serial.println("Lights OFF failed");
                    }
                }
            }
            previous_presence = new_presence;
        }
    }
}

// Periodically performs NTP sync by temporarily reconnecting to Wi-Fi
void RoomNode::NTPSyncTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);

    while (true) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.print("WiFi reconnecting for NTP");
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi reconnected");
        self->ntpClient.initialize();

        WiFi.disconnect();
        Serial.println("WiFi disconnected after NTP sync");
        vTaskDelay(pdMS_TO_TICKS(NTPSYNC_PERIOD));
    }
}

// If initialization or network join fails, sleeps for 30 minutes
void RoomNode::tryLater() {
    esp_sleep_enable_timer_wakeup(30*60*1000000);
    esp_deep_sleep_start();
}