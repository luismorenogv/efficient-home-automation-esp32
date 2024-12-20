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
    : room_id(room_id), wifi_channel(0), communications(), presenceSensor(), lights(), airConditioner(), espnowTaskHandle(NULL) {
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
        LOG_ERROR("ESP-NOW init failed.");
        tryLater();
    }

    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    WiFi.disconnect(); 

    // Set initial mode based on current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    uint16_t current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // Initialize lights state and mode
    if(!lights.initializeState(LDR_PIN)){
        LOG_ERROR("Lights init failed.");
        tryLater();
    }

    // Set default warm/cold schedules
    Time warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};
    Time cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
    lights.setSchedule(warm, cold);
}

// Attempts to join master network by cycling through channels and waiting for ACK
bool RoomNode::joinNetwork() {
    LOG_INFO("Attempting to join network...");

    JoinRoomMsg msg;
    msg.room_id = room_id;
    msg.cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
    msg.warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};

    // If espNowTask has not been created already
    if (espnowTaskHandle == NULL){
        //Start espNow task for ACK message reception
        BaseType_t result;

        result = xTaskCreate(espnowTask, "ESP-NOW Task", 4096, this, 3, &espnowTaskHandle);
        if (result != pdPASS){
            LOG_ERROR("Failed ESP-NOW Task");
            return false;
        }
    }

    

    for (uint8_t i = 0; i < MAX_WIFI_CHANNEL; i++) {
        wifi_channel = (wifi_channel + i) % MAX_WIFI_CHANNEL;
        esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
        communications.registerPeer((uint8_t*)master_mac_addr, wifi_channel);

        connected = false;
        uint8_t retries = 0;
        while (!connected && retries < MAX_RETRIES) {
            communications.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (communications.waitForAck(MessageType::JOIN_ROOM, ACK_TIMEOUT_MS)) {
                connected = true;
            } else {
                retries++;
                LOG_WARNING("No ACK, retry (%u/%u)", retries, MAX_RETRIES);
            }
        }

        if (connected) {
            LOG_INFO("Master on channel %u", wifi_channel);
            return true;
        }
        LOG_WARNING("No ACK in channel %u", wifi_channel);
        communications.unregisterPeer((uint8_t*)master_mac_addr);
    }
    return false;
}

// Creates FreeRTOS tasks for handling communications, lights, presence, and NTP sync
void RoomNode::run() {
    BaseType_t result;

    result = xTaskCreate(presenceTask, "Presence Task", 4096, this, 3, &presenceTaskHandle);
    if (result != pdPASS) LOG_ERROR("Failed Presence Task");
    
    result = xTaskCreate(lightsControlTask, "Lights Control Task", 4096, this, 2, &lightsControlTaskHandle);
    if (result != pdPASS) LOG_ERROR("Failed Lights Task");

    result = xTaskCreate(NTPSyncTask, "NTPSync Task", 4096, this, 1, &NTPSyncTaskHandle);
    if (result != pdPASS) LOG_ERROR("Failed NTPSync Task");

    result = xTaskCreate(heartbeatTask, "Heartbeat Task", 2048, this, 2, &HeartBeatTaskHandle);
    if (result != pdPASS) LOG_ERROR("Failed NTPSync Task");

    LOG_INFO("RoomNode running tasks...");
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
                            LOG_WARNING("Invalid ACK length.");
                            break;
                        }
                        AckMsg* payload = reinterpret_cast<AckMsg*>(msg.data);
                        self->communications.ackReceived(msg.mac_addr, payload->acked_msg);
                    }
                    break;
                case MessageType::NEW_SCHEDULE:
                    {
                        if (msg.len != sizeof(NewScheduleMsg)) {
                            LOG_WARNING("Invalid NEW_SCHEDULE length.");
                            break;
                        }
                        NewScheduleMsg* payload = reinterpret_cast<NewScheduleMsg*>(msg.data);
                        self->lights.setSchedule(payload->warm, payload->cold);
                        self->communications.sendAck(master_mac_addr, MessageType::NEW_SCHEDULE);
                    }
                    break;
                default:
                    LOG_WARNING("Unknown message type: %d", msg_type);
            }
        }
    }
}

// Periodically checks and updates lights mode/brightness
void RoomNode::lightsControlTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    bool lights_on = false;

    while (true) {
        if (self->lights.isOn()){
            time_t now = time(nullptr);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            uint16_t current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
            // Initialize mode each time the lights are turned on
            if (!lights_on){
                self->lights.initializeMode(current_minutes);
                lights_on = true;
            } else {
                self->lights.checkAndUpdateMode(current_minutes);
            }
            self->lights.adjustBrightness(LDR_PIN);
            
        } else {
            lights_on = false;
        }
        vTaskDelay(pdMS_TO_TICKS(LIGHTS_CONTROL_PERIOD));
    }
}

// Monitors presence events from LD2410 and controls lights/AC accordingly
void RoomNode::presenceTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    bool previous_presence = false; // Set initially to false as lights are off initially
    self->presenceSensor.setQueue(self->presenceQueue);
    self->presenceSensor.start();
    while (true) {
        uint8_t presence_state;
        if (xQueueReceive(self->presenceQueue, &presence_state, portMAX_DELAY) == pdTRUE) {
            bool new_presence = (presence_state == HIGH);
            if (new_presence && !previous_presence) {
                LOG_INFO("Presence detected.");
                if (self->lights.isEnoughLight(LDR_PIN)){
                    LOG_INFO("Lights are not necessary --> Enough natural light");
                }
                else{
                    if (self->lights.isOn()){
                        LOG_INFO("Lights are already ON");
                    } else{
                        LOG_INFO("Turning lights ON");
                        CommandResult result = self->lights.sendCommand(Command::ON);
                        if(result == CommandResult::UNCLEAR){
                            LOG_INFO("Lights are unavailable");
                            continue;
                        } else if (result == CommandResult::NEGATIVE){
                            LOG_WARNING("Lights virtual state wasn't synchronized with real state. Fixing issue...");
                            self->lights.sendCommand(Command::ON);
                        } else {
                            LOG_INFO("Lights succesfully turned ON");
                        }
                    }
                }
                
            } else if (!new_presence && previous_presence) {
                LOG_INFO("No presence.");
                self->airConditioner.turnOff();
                if(!self->lights.isOn()){
                    LOG_INFO("Lights are already OFF");
                } else{
                    CommandResult result = self->lights.sendCommand(Command::OFF);
                    if(result == CommandResult::UNCLEAR){
                        LOG_INFO("Lights are unavailable");
                        continue;
                    } else if (result == CommandResult::NEGATIVE){
                        LOG_WARNING("Lights virtual state wasn't synchronized with real state. Fixing issue...");
                        self->lights.sendCommand(Command::OFF);
                    } else{
                        LOG_INFO("Lights succesfully turned OFF");
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
        vTaskDelay(pdMS_TO_TICKS(NTPSYNC_PERIOD));
        WiFi.reconnect();
        LOG_INFO("WiFi reconnecting for NTP");
        while (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(500));
            #ifdef ENABLE_LOGGING
                Serial.print(".");
            #endif
        }
        LOG_INFO("Wi-Fi connected");
        self->ntpClient.initialize();

        WiFi.disconnect();
        LOG_INFO("WiFi disconnected after NTP sync");
        esp_wifi_set_channel(self->wifi_channel, WIFI_SECOND_CHAN_NONE); // Set WiFi channel back to master's channel
    }
}

void RoomNode::heartbeatTask(void* pvParameter){
    RoomNode* self = static_cast<RoomNode*>(pvParameter);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD));
        if (self->connected){
            HeartbeatMsg msg;
            msg.room_id = self->room_id;
            self->communications.sendMsg(reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
            if (!self->communications.waitForAck(MessageType::HEARTBEAT, self->HEARTBEAT_TIMEOUT)){
                self->connected = false;
                LOG_WARNING("HEARTBEAT Ack not received. Trying to reconnect to the network...");
            }
        }
        if (!self->connected){
            self->communications.unregisterPeer((uint8_t*)master_mac_addr);
            if (self->joinNetwork()){
                LOG_INFO("Reconnection successful");
            } else {
                LOG_WARNING("Unable to reconnect to the network");
            }
        }
    }
    
}

// If initialization or network join fails, sleeps for 30 minutes
void RoomNode::tryLater() {
    LOG_INFO("Going deep sleep for the next 30 minutes");
    esp_sleep_enable_timer_wakeup(30*60*1000000);
    esp_deep_sleep_start();
}