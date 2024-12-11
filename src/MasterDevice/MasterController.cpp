/**
 * @file MasterController.cpp
 * @brief Implementation of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/MasterController.h"

// Singleton instance
static MasterController* masterControllerInstance = nullptr;

// Constants for retry mechanism
constexpr uint8_t MAX_RETRIES = 3;
constexpr uint32_t RETRY_INTERVAL_MS = 5000; // 5 seconds

MasterController::MasterController() 
    : communications(), 
      webSockets(dataManager) 
{
    masterControllerInstance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
}

void MasterController::initialize() {
    Serial.begin(115200);
    delay(1000);

    // Initialize communication modules
    communications.initializeWifi();
    if (!communications.initializeESPNOW()) {
        Serial.println("ESP-NOW initialization failed. Entering deep sleep.");
        esp_deep_sleep_start();
    }
    communications.setQueue(espnowQueue);
    
    // Initialize NTP and Web Server
    ntpClient.initialize();
    webServer.initialize();

    // Initialize WebSockets with callbacks
    webSockets.initialize(webServer.getServer());
    webSockets.setSleepDurationCallback(MasterController::sleepPeriodChangedCallback);
    webSockets.setScheduleCallback(MasterController::scheduleChangedCallback);
    
    BaseType_t result;

    // Create ESP-NOW Task
    result = xTaskCreatePinnedToCore(
        espnowTask,
        "ESP-NOW Task",
        8192,
        this,
        2,
        &espnowTaskHandle,
        1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create ESP-NOW Task");
    }

    // Create Web Server Task
    result = xTaskCreatePinnedToCore(
        webServerTask,
        "Web Server Task",
        8192,
        this,
        1,
        &webServerTaskHandle,
        0
    );
    if (result != pdPASS) {
        Serial.println("Failed to create Web Server Task");
    }

    // Create NTP Sync Task
    result = xTaskCreatePinnedToCore(
        ntpSyncTask,
        "NTP Sync Task",
        8192,
        this,
        1,
        &ntpSyncTaskHandle,
        1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create NTP Sync Task");
    }

    // Create Update Check Task
    result = xTaskCreatePinnedToCore(
        updateCheckTask,
        "Update Check Task",
        8192,
        this,
        1,
        nullptr,
        1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create Update Check Task");
    }
}

void MasterController::sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (masterControllerInstance) {
        Serial.printf("Received new sleep period for room %u: %u ms\r\n", room_id, new_sleep_period_ms);
        masterControllerInstance->dataManager.setNewSleepPeriod(room_id, new_sleep_period_ms);
    }
}

void MasterController::scheduleChangedCallback(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min) {
    if (masterControllerInstance) {
        Serial.printf("Received new schedule for room %u: Warm=%02u:%02u, Cold=%02u:%02u\r\n", 
                      room_id, warm_hour, warm_min, cold_hour, cold_min);
        masterControllerInstance->dataManager.setNewSchedule(room_id, warm_hour, warm_min, cold_hour, cold_min);
    }
}

void MasterController::espnowTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    IncomingMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            MessageType msg_type = static_cast<MessageType>(msg.data[0]);
            switch (msg_type) {
                case MessageType::TEMP_HUMID: {
                    TempHumidMsg* payload = reinterpret_cast<TempHumidMsg*>(msg.data);
                    uint8_t room_id = payload->room_id;
                    float temperature = payload->temperature;
                    float humidity = payload->humidity;
                    time_t timestamp = time(nullptr);
                    
                    self->dataManager.addSensorData(room_id, temperature, humidity, timestamp);

                    // Update Web Interface
                    self->webSockets.sendDataUpdate(room_id);

                    // Handle pending sleep period update
                    if (self->dataManager.isPendingUpdate(room_id, NodeType::SENSOR)) {
                        uint32_t new_period = self->dataManager.getNewSleepPeriod(room_id);

                        NewSleepPeriodMsg new_period_msg;
                        new_period_msg.type = MessageType::NEW_SLEEP_PERIOD;
                        new_period_msg.new_period_ms = new_period;

                        uint8_t sensor_mac[MAC_ADDRESS_LENGTH];
                        if (self->dataManager.getMacAddr(room_id, NodeType::SENSOR, sensor_mac)) {
                            self->communications.sendMsg(sensor_mac, reinterpret_cast<uint8_t*>(&new_period_msg), sizeof(NewSleepPeriodMsg));
                            Serial.printf("Sent NEW_SLEEP_PERIOD to sensor in room %u successfully\r\n", room_id);
                            
                            // Update retry mechanism
                            self->pendingSleepUpdate[room_id].attempts++;
                            self->pendingSleepUpdate[room_id].lastAttemptMillis = millis();
                        } else {
                            Serial.printf("Failed to retrieve MAC address for room %u. Cannot send NEW_SLEEP_PERIOD.\r\n", room_id);
                        }
                    } else {
                        // Acknowledge TEMP_HUMID message
                        self->communications.sendAck(msg.mac_addr, MessageType::TEMP_HUMID);
                    }
                }
                break;

                case MessageType::JOIN_SENSOR: {
                    JoinSensorMsg* payload = reinterpret_cast<JoinSensorMsg*>(msg.data);
                    uint8_t room_id = payload->room_id;
                    uint32_t sleep_period_ms = payload->sleep_period_ms;
                    
                    self->dataManager.sensorSetup(room_id, msg.mac_addr, sleep_period_ms);

                    Serial.printf("Received JOIN_SENSOR from room %u with sleep_period %u ms\r\n", room_id, sleep_period_ms);
                    self->communications.registerPeer(msg.mac_addr, WiFi.channel());
                    self->communications.sendAck(msg.mac_addr, msg_type);
                }
                break;

                case MessageType::ACK: {
                    if (msg.len < sizeof(AckMsg)) {
                        Serial.println("Received malformed ACK message.");
                        break;
                    }
                    AckMsg* payload = reinterpret_cast<AckMsg*>(msg.data);
                    
                    if (payload->acked_msg == MessageType::NEW_SLEEP_PERIOD) {
                        uint8_t room_id = self->dataManager.getId(msg.mac_addr);
                        if (room_id != ID_NOT_VALID) {
                            self->dataManager.sleepPeriodWasUpdated(room_id);
                            self->pendingSleepUpdate[room_id].attempts = 0;
                            Serial.printf("Received ACK for NEW_SLEEP_PERIOD from room %u\r\n", room_id);
                        } else {
                            Serial.println("Received ACK for NEW_SLEEP_PERIOD from unknown SensorNode.");
                        }
                    } else if (payload->acked_msg == MessageType::NEW_SCHEDULE) {
                        uint8_t room_id = self->dataManager.getId(msg.mac_addr);
                        if (room_id != ID_NOT_VALID) {
                            self->dataManager.scheduleWasUpdated(room_id);
                            self->pendingScheduleUpdate[room_id].attempts = 0;
                            Serial.printf("Received ACK for NEW_SCHEDULE from room %u\r\n", room_id);
                        } else {
                            Serial.println("Received ACK for NEW_SCHEDULE from unknown RoomNode.");
                        }
                    } else {
                        Serial.printf("Received ACK for unknown MessageType: %d\r\n", payload->acked_msg);
                    }
                }
                break;

                case MessageType::JOIN_ROOM: {
                    JoinRoomMsg* payload = reinterpret_cast<JoinRoomMsg*>(msg.data);
                    uint8_t room_id = payload->room_id;
                    
                    self->dataManager.controlSetup(room_id, msg.mac_addr, 
                                                   payload->warm.hour, payload->warm.min, 
                                                   payload->cold.hour, payload->cold.min);
                    
                    self->communications.registerPeer(msg.mac_addr, WiFi.channel());
                    self->communications.sendAck(msg.mac_addr, MessageType::JOIN_ROOM);
                    
                    Serial.printf("Received JOIN_ROOM from room %u with warm/cold times\r\n", room_id);

                    // Update Web Interface
                    self->webSockets.sendDataUpdate(room_id);
                }
                break;

                default:
                    Serial.printf("Received unknown message type: %d\r\n", msg_type);
            }
        }
    }
}

void MasterController::webServerTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    self->webServer.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(WEB_SERVER_PERIOD));
    }
}

void MasterController::ntpSyncTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(NTPSYNC_PERIOD));
        Serial.println("Re-synchronizing NTP time...");
        self->ntpClient.initialize();
    }
}

void MasterController::updateCheckTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CHECK_PENDING_MSG_PERIOD));
        self->checkAndResendUpdates();
    }
}

void MasterController::checkAndResendUpdates() {
    for (uint8_t i = 0; i < NUM_ROOMS; i++) {
        // Handle sleep period updates
        if (dataManager.isPendingUpdate(i, NodeType::SENSOR)) {
            uint32_t nowMs = millis();
            if (pendingSleepUpdate[i].attempts < MAX_RETRIES && 
                (nowMs - pendingSleepUpdate[i].lastAttemptMillis > RETRY_INTERVAL_MS)) {
                
                uint32_t new_period = dataManager.getNewSleepPeriod(i);
                NewSleepPeriodMsg new_period_msg;
                new_period_msg.type = MessageType::NEW_SLEEP_PERIOD;
                new_period_msg.new_period_ms = new_period;

                uint8_t sensor_mac[MAC_ADDRESS_LENGTH];
                if (dataManager.getMacAddr(i, NodeType::SENSOR, sensor_mac)) {
                    communications.sendMsg(sensor_mac, reinterpret_cast<uint8_t*>(&new_period_msg), sizeof(NewSleepPeriodMsg));
                    Serial.printf("Resent NEW_SLEEP_PERIOD to sensor in room %u (attempt %u)\r\n", 
                                  i, pendingSleepUpdate[i].attempts + 1);
                    pendingSleepUpdate[i].attempts++;
                    pendingSleepUpdate[i].lastAttemptMillis = nowMs;
                } else {
                    Serial.printf("Failed to retrieve MAC address for room %u. Cannot resend NEW_SLEEP_PERIOD.\r\n", i);
                }
            }
        } else {
            // Reset attempts if no longer pending
            pendingSleepUpdate[i].attempts = 0;
        }

        // Handle schedule updates
        if (dataManager.isPendingUpdate(i, NodeType::ROOM)) {
            uint32_t nowMs = millis();
            if (pendingScheduleUpdate[i].attempts < MAX_RETRIES && 
                (nowMs - pendingScheduleUpdate[i].lastAttemptMillis > RETRY_INTERVAL_MS)) {
                
                RoomData rd = dataManager.getRoomData(i);
                uint8_t room_mac[MAC_ADDRESS_LENGTH];
                if (dataManager.getMacAddr(i, NodeType::ROOM, room_mac)) {
                    NewScheduleMsg scheduleMsg;
                    scheduleMsg.type = MessageType::NEW_SCHEDULE;
                    scheduleMsg.warm = rd.control.warm;
                    scheduleMsg.cold = rd.control.cold;

                    communications.sendMsg(room_mac, reinterpret_cast<uint8_t*>(&scheduleMsg), sizeof(scheduleMsg));
                    Serial.printf("Resent NEW_SCHEDULE to room %u (attempt %u)\r\n", 
                                  i, pendingScheduleUpdate[i].attempts + 1);
                    pendingScheduleUpdate[i].attempts++;
                    pendingScheduleUpdate[i].lastAttemptMillis = nowMs;
                }
            }
        } else {
            // Reset attempts if no longer pending
            pendingScheduleUpdate[i].attempts = 0;
        }
    }
}
