/**
 * @file MasterController.cpp
 * @brief Implementation of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/MasterController.h"

// Singleton instance
static MasterController* instance = nullptr;

MasterController::MasterController() 
    : communications(), 
      webSockets(dataManager) 
{
    instance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
}

void MasterController::initialize() {
    Serial.begin(115200);
    delay(1000);

    // If needed
    // setCpuFrequencyMhz(240);

    // Initialize communication modules
    communications.initializeWifi();
    if (!communications.initializeESPNOW()) {
        LOG_ERROR("ESP-NOW initialization failed. Entering deep sleep.");
        tryLater();
    }
    communications.setQueue(espnowQueue);
    
    // Initialize NTP and Web Server
    while(!ntpClient.initialize());
    if (!webServer.initialize()){
        tryLater();
    }

    // Initialize WebSockets with callbacks
    webSockets.initialize(webServer.getServer());
    webSockets.setSleepDurationCallback(MasterController::sleepPeriodChangedCallback);
    webSockets.setScheduleCallback(MasterController::scheduleChangedCallback);
    webSockets.setLightsToggleCallback(MasterController::lightsToggleCallback);
    
    BaseType_t result;

    // Create ESP-NOW Task
    result = xTaskCreatePinnedToCore(espnowTask,"ESP-NOW Task",8192,this,2,&espnowTaskHandle,1);
    if (result != pdPASS) {
        LOG_ERROR("Failed to create ESP-NOW Task");
    }

    // Create Web Server Task
    result = xTaskCreatePinnedToCore(webServerTask,"Web Server Task",8192,this,2,&webServerTaskHandle,0);
    if (result != pdPASS) {
        LOG_ERROR("Failed to create Web Server Task");
    }

    // Create NTP Sync Task
    result = xTaskCreatePinnedToCore(ntpSyncTask,"NTP Sync Task",8192,this,1,&ntpSyncTaskHandle,1);
    if (result != pdPASS) {
        LOG_ERROR("Failed to create NTP Sync Task");
    }

    // Create Update Check Task
    result = xTaskCreatePinnedToCore(updateCheckTask,"Update Check Task",8192,this,1,nullptr,1);
    if (result != pdPASS) {
        LOG_ERROR("Failed to create Update Check Task");
    }
}

void MasterController::sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (instance) {
        LOG_INFO("Received new sleep period for room %u: %u ms", room_id, new_sleep_period_ms);
        instance->dataManager.setNewSleepPeriod(room_id, new_sleep_period_ms);
    }
}

void MasterController::scheduleChangedCallback(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min) {
    if (instance) {
        LOG_INFO("Received new schedule for room %u: Warm=%02u:%02u, Cold=%02u:%02u", 
                      room_id, warm_hour, warm_min, cold_hour, cold_min);
        instance->dataManager.setNewSchedule(room_id, warm_hour, warm_min, cold_hour, cold_min);
        NewScheduleMsg scheduleMsg;
        scheduleMsg.type = MessageType::NEW_SCHEDULE;
        scheduleMsg.warm = {warm_hour, warm_min};
        scheduleMsg.cold = {cold_hour, cold_min};
        uint8_t dest_mac[MAC_ADDRESS_LENGTH];
        if(instance->dataManager.getMacAddr(room_id, NodeType::ROOM, dest_mac)){
            instance->communications.sendMsg(dest_mac, reinterpret_cast<uint8_t*>(&scheduleMsg), sizeof(scheduleMsg));
            LOG_INFO("Sent NEW_SCHEDULE to room %u", 
                        room_id);
            instance->pendingScheduleUpdate[room_id].attempts++;
            instance->pendingScheduleUpdate[room_id].lastAttemptMillis = millis();;
        } else {
            LOG_ERROR("Failed to get MAC address for room %u. Cannot send NEW_SCHEDULE.", room_id);
        }
        
    }
}

void MasterController::lightsToggleCallback(uint8_t room_id, bool turn_on) {
    if (instance) {
        LightsToggleMsg toggleMsg;
        toggleMsg.type = MessageType::LIGHTS_TOGGLE;
        toggleMsg.turn_on = turn_on;

        uint8_t room_mac[MAC_ADDRESS_LENGTH];
        if (instance->dataManager.getMacAddr(room_id, NodeType::ROOM, room_mac)) {
            instance->communications.sendMsg(room_mac, reinterpret_cast<uint8_t*>(&toggleMsg), sizeof(LightsToggleMsg));
            LOG_INFO("Sent LIGHTS_TOGGLE to room %u: %s", room_id, turn_on ? "ON" : "OFF");
        } else {
            LOG_ERROR("Failed to get MAC address for room %u. Cannot toggle lights.", room_id);
        }
    }
}

// Handles incoming ESP-NOW messages from master
void MasterController::espnowTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    IncomingMsg msg;
    MessageType msg_type;

    // Declare variables outside the loop to reduce stack usage
    TempHumidMsg* payload_temp_humid = nullptr;
    JoinSensorMsg* payload_join_sensor = nullptr;
    AckMsg* payload_ack = nullptr;
    JoinRoomMsg* payload_join_room = nullptr;
    HeartbeatMsg* payload_heartbeat = nullptr;
    LightsUpdateMsg* payload_lights_update = nullptr;
    NewSleepPeriodMsg new_period_msg;
    uint8_t room_id;
    float temperature;
    float humidity;
    time_t timestamp;
    uint32_t new_period;
    uint8_t sensor_mac[MAC_ADDRESS_LENGTH];
    bool is_on;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            msg_type = static_cast<MessageType>(msg.data[0]);
            switch (msg_type) {
                case MessageType::TEMP_HUMID: {
                    if (msg.len != sizeof(TempHumidMsg)) {
                        LOG_WARNING("Received malformed TEMP_HUMID message.");
                        break;
                    }
                    payload_temp_humid = reinterpret_cast<TempHumidMsg*>(msg.data);
                    room_id = payload_temp_humid->room_id;
                    temperature = payload_temp_humid->temperature;
                    humidity = payload_temp_humid->humidity;
                    timestamp = time(nullptr);
                    
                    self->dataManager.addSensorData(room_id, temperature, humidity, timestamp);

                    // Update Web Interface
                    self->webSockets.sendDataUpdate(room_id);

                    // Handle pending sleep period update
                    if (self->dataManager.isPendingUpdate(room_id, NodeType::SENSOR)) {
                        new_period = self->dataManager.getNewSleepPeriod(room_id);

                        new_period_msg.type = MessageType::NEW_SLEEP_PERIOD;
                        new_period_msg.new_period_ms = new_period;

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
                    if (msg.len != sizeof(JoinSensorMsg)) {
                        LOG_WARNING("Received malformed JOIN_SENSOR message.");
                        break;
                    }
                    payload_join_sensor = reinterpret_cast<JoinSensorMsg*>(msg.data);
                    room_id = payload_join_sensor->room_id;
                    uint32_t sleep_period_ms = payload_join_sensor->sleep_period_ms;
                    
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
                    payload_ack = reinterpret_cast<AckMsg*>(msg.data);
                    
                    uint8_t acked_room_id = self->dataManager.getId(msg.mac_addr);
                    if (acked_room_id == ID_NOT_VALID) {
                        LOG_WARNING("Received ACK from unknown node.");
                        break;
                    }

                    if (payload_ack->acked_msg == MessageType::NEW_SLEEP_PERIOD) {
                        self->dataManager.sleepPeriodWasUpdated(acked_room_id);
                        self->pendingSleepUpdate[acked_room_id].attempts = 0;
                        self->webSockets.sendDataUpdate(acked_room_id);
                        LOG_INFO("Received ACK for NEW_SLEEP_PERIOD from room %u", acked_room_id);
                    } else if (payload_ack->acked_msg == MessageType::NEW_SCHEDULE) {
                        self->dataManager.scheduleWasUpdated(acked_room_id);
                        self->pendingScheduleUpdate[acked_room_id].attempts = 0;
                        self->webSockets.sendDataUpdate(acked_room_id);
                        LOG_INFO("Received ACK for NEW_SCHEDULE from room %u", acked_room_id);
                    } else {
                        LOG_WARNING("Received ACK for unknown MessageType: %d", payload_ack->acked_msg);
                    }
                }
                break;

                case MessageType::JOIN_ROOM: {
                    if (msg.len != sizeof(JoinRoomMsg)) {
                        LOG_WARNING("Received malformed JOIN_ROOM message.");
                        break;
                    }
                    payload_join_room = reinterpret_cast<JoinRoomMsg*>(msg.data);
                    room_id = payload_join_room->room_id;
                    self->communications.registerPeer(msg.mac_addr, WiFi.channel());
                    self->communications.sendAck(msg.mac_addr, MessageType::JOIN_ROOM);
                    self->dataManager.controlSetup(room_id, msg.mac_addr, payload_join_room->lights_on, 
                                                   payload_join_room->warm.hour, payload_join_room->warm.min, 
                                                   payload_join_room->cold.hour, payload_join_room->cold.min);
                    LOG_INFO("Received JOIN_ROOM from room %u with warm/cold times", room_id);

                    // Update Web Interface
                    self->webSockets.sendDataUpdate(room_id);
                }
                break;

                case MessageType::HEARTBEAT: {
                    if (msg.len != sizeof(HeartbeatMsg)) {
                        LOG_WARNING("Received malformed HEARTBEAT message.");
                        break;
                    }
                    payload_heartbeat = reinterpret_cast<HeartbeatMsg*>(msg.data);
                    room_id = payload_heartbeat->room_id;
                    if (self->dataManager.isRegistered(room_id, NodeType::ROOM)){
                        self->dataManager.updateHeartbeat(room_id);
                        self->communications.sendAck(msg.mac_addr, MessageType::HEARTBEAT);
                    } else {
                        LOG_WARNING("Heartbeat received from unregistered device");
                    }
                    
                }
                break;

                case MessageType::LIGHTS_UPDATE: {
                    if (msg.len != sizeof(LightsUpdateMsg)) {
                        LOG_WARNING("Received malformed LIGHTS_UPDATE message.");
                        break;
                    }
                    payload_lights_update = reinterpret_cast<LightsUpdateMsg*>(msg.data);
                    room_id = self->dataManager.getId(msg.mac_addr);
                    if (room_id != ID_NOT_VALID) {
                        is_on = payload_lights_update->is_on;
                        self->dataManager.setLightsOn(room_id, is_on);
                        LOG_INFO("Room %u reports lights are now %s", room_id, is_on ? "ON" : "OFF");

                        // Update Web Interface
                        self->webSockets.sendDataUpdate(room_id);
                    } else {
                        LOG_WARNING("LIGHTS_UPDATE from unknown node");
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
                        scheduleMsg.warm = rd.control.new_warm;
                        scheduleMsg.cold = rd.control.new_cold;

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
                webSockets.sendDataUpdate(i);
            }
        }
    }
}

void MasterController::tryLater(){
    esp_sleep_enable_timer_wakeup(30*60*1000000);
    esp_deep_sleep_start();
}