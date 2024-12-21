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
        LOG_ERROR("ESP-NOW initialization failed. Entering deep sleep.");
        tryLater();
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
        LOG_ERROR("Failed to create ESP-NOW Task");
    }

    // Create Web Server Task
    result = xTaskCreatePinnedToCore(
        webServerTask,
        "Web Server Task",
        8192,
        this,
        2,
        &webServerTaskHandle,
        0
    );
    if (result != pdPASS) {
        LOG_ERROR("Failed to create Web Server Task");
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
        LOG_ERROR("Failed to create NTP Sync Task");
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
        LOG_ERROR("Failed to create Update Check Task");
    }
}

void MasterController::sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (masterControllerInstance) {
        LOG_INFO("Received new sleep period for room %u: %u ms", room_id, new_sleep_period_ms);
        masterControllerInstance->dataManager.setNewSleepPeriod(room_id, new_sleep_period_ms);
    }
}

void MasterController::scheduleChangedCallback(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min) {
    if (masterControllerInstance) {
        LOG_INFO("Received new schedule for room %u: Warm=%02u:%02u, Cold=%02u:%02u", 
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
                            LOG_INFO("Sent NEW_SLEEP_PERIOD to sensor in room %u successfully", room_id);
                            
                            // Update retry mechanism
                            self->pendingSleepUpdate[room_id].attempts++;
                            self->pendingSleepUpdate[room_id].lastAttemptMillis = millis();
                            if (self->pendingSleepUpdate[room_id].attempts > MAX_RETRIES){
                                LOG_WARNING("Communication with sensorNode with ID %u isn't working as expected.", room_id);
                            }
                        } else {
                            LOG_ERROR("Failed to retrieve MAC address for room %u. Cannot send NEW_SLEEP_PERIOD.", room_id);
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

                    LOG_INFO("Received JOIN_SENSOR from room %u with sleep_period %u ms", room_id, sleep_period_ms);
                    self->communications.registerPeer(msg.mac_addr, WiFi.channel());
                    self->communications.sendAck(msg.mac_addr, msg_type);
                }
                break;

                case MessageType::ACK: {
                    if (msg.len < sizeof(AckMsg)) {
                        LOG_WARNING("Received malformed ACK message.");
                        break;
                    }
                    AckMsg* payload = reinterpret_cast<AckMsg*>(msg.data);
                    
                    if (payload->acked_msg == MessageType::NEW_SLEEP_PERIOD) {
                        uint8_t room_id = self->dataManager.getId(msg.mac_addr);
                        if (room_id != ID_NOT_VALID) {
                            self->dataManager.sleepPeriodWasUpdated(room_id);
                            self->pendingSleepUpdate[room_id].attempts = 0;
                            LOG_INFO("Received ACK for NEW_SLEEP_PERIOD from room %u", room_id);
                        } else {
                            LOG_WARNING("Received ACK for NEW_SLEEP_PERIOD from unknown SensorNode.");
                        }
                    } else if (payload->acked_msg == MessageType::NEW_SCHEDULE) {
                        uint8_t room_id = self->dataManager.getId(msg.mac_addr);
                        if (room_id != ID_NOT_VALID) {
                            self->dataManager.scheduleWasUpdated(room_id);
                            self->pendingScheduleUpdate[room_id].attempts = 0;
                            LOG_INFO("Received ACK for NEW_SCHEDULE from room %u", room_id);
                        } else {
                            LOG_WARNING("Received ACK for NEW_SCHEDULE from unknown RoomNode.");
                        }
                    } else {
                        LOG_WARNING("Received ACK for unknown MessageType: %d", payload->acked_msg);
                    }
                }
                break;

                case MessageType::JOIN_ROOM: {
                    JoinRoomMsg* payload = reinterpret_cast<JoinRoomMsg*>(msg.data);
                    uint8_t room_id = payload->room_id;
                    self->communications.registerPeer(msg.mac_addr, WiFi.channel());
                    self->communications.sendAck(msg.mac_addr, MessageType::JOIN_ROOM);
                    self->dataManager.controlSetup(room_id, msg.mac_addr, 
                                                   payload->warm.hour, payload->warm.min, 
                                                   payload->cold.hour, payload->cold.min);
                    LOG_INFO("Received JOIN_ROOM from room %u with warm/cold times", room_id);

                    // Update Web Interface
                    self->webSockets.sendDataUpdate(room_id);
                }
                break;

                case MessageType::HEARTBEAT: {
                    HeartbeatMsg* payload = reinterpret_cast<HeartbeatMsg*>(msg.data);
                    uint8_t room_id = payload->room_id;
                    if (self->dataManager.isRegistered(room_id, NodeType::ROOM)){
                        self->dataManager.updateHeartbeat(room_id);
                        self->communications.sendAck(msg.mac_addr, MessageType::HEARTBEAT);
                    } else {
                        LOG_WARNING("Heartbeat received from unregistered device");
                    }
                    
                }
                break;

                default:
                    LOG_WARNING("Received unknown message type: %d", msg_type);
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
        LOG_INFO("Re-synchronizing NTP time...");
        self->ntpClient.initialize();
    }
}

void MasterController::updateCheckTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CHECK_PENDING_MSG_PERIOD));
        self->checkAndResendUpdates();
        self->checkHeartbeats();
        self->checkSensorNodes();
    }
}

void MasterController::checkAndResendUpdates() {
    for (uint8_t i = 0; i < NUM_ROOMS; i++) {
        // Handle schedule updates
        if (dataManager.isPendingUpdate(i, NodeType::ROOM)) {
            uint32_t nowMs = millis();
            if ((nowMs - pendingScheduleUpdate[i].lastAttemptMillis > RETRY_INTERVAL_MS)){
                if (pendingScheduleUpdate[i].attempts < MAX_RETRIES){
                    RoomData rd = dataManager.getRoomData(i);
                    uint8_t room_mac[MAC_ADDRESS_LENGTH];
                    if (dataManager.getMacAddr(i, NodeType::ROOM, room_mac)) {
                        NewScheduleMsg scheduleMsg;
                        scheduleMsg.type = MessageType::NEW_SCHEDULE;
                        scheduleMsg.warm = rd.control.warm;
                        scheduleMsg.cold = rd.control.cold;

                        communications.sendMsg(room_mac, reinterpret_cast<uint8_t*>(&scheduleMsg), sizeof(scheduleMsg));
                        LOG_INFO("Resent NEW_SCHEDULE to room %u (attempt %u)", 
                                    i, pendingScheduleUpdate[i].attempts + 1);
                        pendingScheduleUpdate[i].attempts++;
                        pendingScheduleUpdate[i].lastAttemptMillis = nowMs;
                    }
                } else {
                    LOG_WARNING("RoomNode with ID %u is not responding to new schedule update", i);
                    dataManager.unregisterNode(i, NodeType::ROOM);
                    LOG_INFO("Unregistered roomNode with ID: %u", i);
                    webSockets.sendDataUpdate(i);
                }
            }
        }
    }
}

void MasterController::checkHeartbeats(){
    for (uint8_t i = 0; i < NUM_ROOMS; i++){
        if (dataManager.isRegistered(i, NodeType::ROOM) && millis() - dataManager.getLatestHeartbeat(i) > HEARTBEAT_TIMEOUT){
            LOG_WARNING("Heartbeat from RoomNode with ID %u not received in time", i);
            dataManager.unregisterNode(i, NodeType::ROOM);
            LOG_INFO("RoomNode with ID %u has been unregistered", i);
            webSockets.sendDataUpdate(i);
        }
    }
}

void MasterController::checkSensorNodes(){
    for (uint8_t i = 0; i < NUM_ROOMS; i++){
        if (dataManager.isRegistered(i, NodeType::SENSOR)){
            if (!dataManager.checkIfSensorActive(i)){
                LOG_WARNING("Data from SensorNode with ID %u not received in time", i);
                dataManager.unregisterNode(i, NodeType::SENSOR);
                LOG_INFO("SensorNode with ID %u has been unregistered", i);
            }
        }
    }
}

void MasterController::tryLater(){
    esp_sleep_enable_timer_wakeup(30*60*1000000);
    esp_deep_sleep_start();
}