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
    : room_id(room_id), wifi_channel(0), presenceSensor(), communications(&radioMutex), lights(), airConditioner(),
    espnowTaskHandle(NULL), user_stop(false), connected(false), cold({DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD}), warm({DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM}) {
    instance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
    presenceQueue = xQueueCreate(10, sizeof(uint8_t));
    radioMutex = xSemaphoreCreateMutex();
}

// Initializes node: sets up Wi-Fi, ESP-NOW, presence sensor, NTP, and lights schedule
void RoomNode::initialize() {
    Serial.begin(115200);
    delay(1000);

    if (!presenceSensor.initialize()){
        LOG_ERROR("Presence Sensor init failed.");
        // tryLater();
    }

    communications.initializeWifi();
    while(!ntpClient.initialize());
    if (!communications.initializeESPNOW()) {
        LOG_ERROR("ESP-NOW init failed.");
        tryLater();
    }

    communications.setQueue(espnowQueue);
    
    WiFi.disconnect(); 

    if(!joinNetwork()){
        LOG_WARNING("No initial connection to Master. RommNode will trying conneting through execution");
    }

    // Set initial mode based on current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    uint16_t current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // Initialize lights state and mode
    if(!lights.initializeState()){
        LOG_ERROR("Lights init failed.");
        tryLater();
    }

    // Set default warm/cold schedules
    lights.setSchedule(warm, cold);
}

// Attempts to join master network by cycling through channels and waiting for ACK
bool RoomNode::joinNetwork() {
    LOG_INFO("Attempting to join network...");

    JoinRoomMsg msg;
    msg.room_id = room_id;
    msg.cold = cold;
    msg.warm = warm;
    msg.lights_on = lights.isOn();

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
            if (communications.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg))){
                if (communications.waitForAck(MessageType::JOIN_ROOM, ACK_TIMEOUT_MS)) {
                    connected = true;
                } else {
                    retries++;
                    LOG_WARNING("No ACK, retry (%u/%u)", retries, MAX_RETRIES);
                }
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
    MessageType msg_type;

    // Declare variables outside the loop to minimize stack usage
    AckMsg* ack_payload = nullptr;
    NewScheduleMsg* schedule_payload = nullptr;
    LightsToggleMsg* toggle_payload = nullptr;
    LightsUpdateMsg lights_update;
    uint8_t presence_state;
    bool turn_on;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            msg_type = static_cast<MessageType>(msg.data[0]);

            switch (msg_type) {
                case MessageType::ACK:
                    {
                        if (msg.len != sizeof(AckMsg)) {
                            LOG_WARNING("Invalid ACK length.");
                            break;
                        }
                        ack_payload = reinterpret_cast<AckMsg*>(msg.data);
                        self->communications.ackReceived(msg.mac_addr, ack_payload->acked_msg);
                    }
                    break;

                case MessageType::NEW_SCHEDULE:
                    {
                        if (msg.len != sizeof(NewScheduleMsg)) {
                            LOG_WARNING("Invalid NEW_SCHEDULE length.");
                            break;
                        }
                        if (!self->connected){
                            LOG_WARNING("NEW_SCHEDULE message is not expected");
                            break;
                        }
                        schedule_payload = reinterpret_cast<NewScheduleMsg*>(msg.data);
                        self->lights.setSchedule(schedule_payload->warm, schedule_payload->cold);
                        self->communications.sendAck(master_mac_addr, MessageType::NEW_SCHEDULE);
                    }
                    break;

                case MessageType::LIGHTS_TOGGLE:
                    {
                        if (msg.len != sizeof(LightsToggleMsg)) {
                            LOG_WARNING("Invalid LIGHTS_TOGGLE length.");
                            break;
                        }
                        if (!self->connected){
                            LOG_WARNING("LIGHTS_TOGGLE message is not expected");
                            break;
                        }
                        toggle_payload = reinterpret_cast<LightsToggleMsg*>(msg.data);
                        turn_on = toggle_payload->turn_on;

                        // User can turn off lights at any presence state and it will pause the presence automation
                        // But user can only turn on lights if presence is detected
                        if (turn_on) {
                            if (self->lights.isOn()) {
                                LOG_INFO("Lights are already ON");
                            } else {
                                LOG_INFO("Turning lights ON");
                                presence_state = digitalRead(LD2410_PIN);
                                if (presence_state == HIGH){
                                    self->lights.sendCommand(Command::ON);
                                    self->user_stop = false;
                                } else {
                                    // presenceTask will turn on the lights if presence is detected
                                    LOG_INFO("Presence not detected. Lights turn on ignored.");
                                }
                            }
                        } else {
                            if (!self->lights.isOn()) {
                                LOG_INFO("Lights are already OFF");
                            } else {
                                LOG_INFO("Turning lights OFF");
                                self->lights.sendCommand(Command::OFF);
                            }
                            self->user_stop = true;
                        }
                        lights_update.is_on = self->lights.isOn(); 
                        LOG_INFO("TEST2");
                        self->communications.sendMsg(reinterpret_cast<const uint8_t*>(&lights_update), sizeof(lights_update));
                        LOG_INFO("Lights update sent to the master");
                    }
                    break;

                default:
                    LOG_WARNING("Unknown message type: %d", msg_type);
            }
            // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            // LOG_INFO("espNowTask: Stack watermark = %u", (unsigned)watermark);

        }
    }
}


// Periodically checks and updates lights mode/brightness
void RoomNode::lightsControlTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    bool lights_on = false;
    time_t now;
    uint16_t current_minutes;
    CommandResult result;
    LightsUpdateMsg lights_update;
    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait presenceTask to execute

    while (true) {
        // Lock task when user manually turns off lights
        while(self->user_stop){
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if (self->lights.isOn()){
            now = time(nullptr);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            current_minutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
            // Initialize mode each time the lights are turned on
            if (!lights_on){
                self->lights.initializeMode(current_minutes);
                lights_on = true;
            } else {
                self->lights.checkAndUpdateMode(current_minutes);
            }
            self->lights.adjustBrightness();
            
        } else {
            if (!self->lights.isEnoughLight() && self->presenceSensor.getPresence()){
                LOG_INFO("There is not enought natural light anymore, turning on lights");
                self->lights.sendCommand(Command::ON);
                result = self->lights.sendCommand(Command::ON);
                if(result == CommandResult::UNCLEAR){
                    LOG_INFO("Lights are unavailable");
                } else if (result == CommandResult::NEGATIVE){
                    LOG_WARNING("Lights virtual state wasn't synchronized with real state. Fixing issue...");
                    self->lights.sendCommand(Command::ON);
                } else {
                    lights_update.is_on = true;
                    self->communications.sendMsg(reinterpret_cast<const uint8_t*>(&lights_update), sizeof(LightsUpdateMsg));
                    LOG_INFO("Lights succesfully turned ON");
                }
            } else {
                lights_on = false;
            }
        }
        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // LOG_INFO("lightsControlTask: Stack watermark = %u", (unsigned)watermark);

        vTaskDelay(pdMS_TO_TICKS(LIGHTS_CONTROL_PERIOD));
    }
}

// Monitors presence events from LD2410 and controls lights/AC accordingly
void RoomNode::presenceTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    bool previous_presence = false; // Set initially to false as lights are off initially
    uint8_t presence_state;
    bool new_presence;
    CommandResult result;
    LightsUpdateMsg lights_update;
    self->presenceSensor.setQueue(self->presenceQueue);
    self->presenceSensor.start();
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for sensor to stabilize
    while (true) {
        if (xQueueReceive(self->presenceQueue, &presence_state, portMAX_DELAY) == pdTRUE) {
            if (self->user_stop) {
                // Discart event avoiding queue overflow
                continue;
            }

            new_presence = (presence_state == HIGH);
            if (new_presence && !previous_presence) {
                LOG_INFO("Presence detected.");
                if (self->lights.isOn()){
                    LOG_INFO("Lights are already ON");
                } else{
                    if (self->lights.isEnoughLight()){
                        LOG_INFO("Lights are OFF but with enough natural light. Leaving them OFF");
                    } else {
                        LOG_INFO("Turning lights ON");
                        result = self->lights.sendCommand(Command::ON);
                        if(result == CommandResult::UNCLEAR){
                            LOG_INFO("Lights are unavailable");
                            continue;
                        } else if (result == CommandResult::NEGATIVE){
                            LOG_WARNING("Lights virtual state wasn't synchronized with real state. Fixing issue...");
                            self->lights.sendCommand(Command::ON);
                        } else {
                            lights_update.is_on = true;
                            self->communications.sendMsg(reinterpret_cast<const uint8_t*>(&lights_update), sizeof(LightsUpdateMsg));
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
                    result = self->lights.sendCommand(Command::OFF);
                    if(result == CommandResult::UNCLEAR){
                        LOG_INFO("Lights are unavailable");
                        continue;
                    } else if (result == CommandResult::NEGATIVE){
                        LOG_WARNING("Lights virtual state wasn't synchronized with real state. Fixing issue...");
                        self->lights.sendCommand(Command::OFF);
                    } else{
                        LOG_INFO("Lights succesfully turned OFF");
                        lights_update.is_on = false;
                        self->communications.sendMsg(reinterpret_cast<const uint8_t*>(&lights_update), sizeof(lights_update));
                    }
                }
            }
            previous_presence = new_presence;
            // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            // LOG_INFO("presenceTask: Stack watermark = %u", (unsigned)watermark);

        }
    }
}

// Periodically performs NTP sync by temporarily reconnecting to Wi-Fi
void RoomNode::NTPSyncTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(NTPSYNC_PERIOD));
        // Semaphore is needed because router may assign a different channel from the MasterDevice
        xSemaphoreTake(self->radioMutex, portMAX_DELAY);
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
        xSemaphoreGive(self->radioMutex);
        
        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // LOG_INFO("NTPSyncTask: Stack watermark = %u", (unsigned)watermark);

    }
}

void RoomNode::heartbeatTask(void* pvParameter){
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    HeartbeatMsg msg;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD));
        if (self->connected){
            msg.room_id = self->room_id;
            if (self->communications.sendMsg(reinterpret_cast<uint8_t*>(&msg), sizeof(msg))){
                if (!self->communications.waitForAck(MessageType::HEARTBEAT, self->HEARTBEAT_TIMEOUT)){
                    self->connected = false;
                    LOG_WARNING("HEARTBEAT Ack not received. Trying to reconnect to the network...");
                }
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
        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // LOG_INFO("heartbeatTask: Stack watermark = %u", (unsigned)watermark);
    }
    
}

// If initialization or network join fails, sleeps for 30 minutes
void RoomNode::tryLater() {
    LOG_INFO("Going deep sleep for the next 30 minutes");
    esp_sleep_enable_timer_wakeup(30*60*1000000);
    esp_deep_sleep_start();
}