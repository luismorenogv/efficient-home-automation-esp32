/**
 * @file DataManager.cpp
 * @brief Implementation of DataManager class for managing sensor data storage and retrieval
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/DataManager.h"

// Constructor initializes mutexes for thread-safe operations
DataManager::DataManager() {
    sensorMutex = xSemaphoreCreateMutex();
    controlMutex = xSemaphoreCreateMutex();
}

void DataManager::addSensorData(uint8_t room_id, float temperature, float humidity, time_t timestamp) {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            RoomData& room = rooms[room_id];
            uint16_t idx = room.sensor.index;
            room.sensor.temperature[idx] = temperature;
            room.sensor.humidity[idx] = humidity;
            room.sensor.timestamps[idx] = timestamp;
            room.sensor.valid_data_points++;
            if (room.sensor.valid_data_points > MAX_DATA_POINTS) {
                room.sensor.valid_data_points = MAX_DATA_POINTS;
            }
            // Update index for circular buffer
            room.sensor.index = (idx + 1) % MAX_DATA_POINTS;
        xSemaphoreGive(sensorMutex);
    }
}

void DataManager::setNewSleepPeriod(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            if (rooms[room_id].sensor.sleep_period_ms != new_sleep_period_ms){
                rooms[room_id].sensor.new_sleep_period_ms = new_sleep_period_ms;
                rooms[room_id].sensor.pending_update = true;
            }
        xSemaphoreGive(sensorMutex);
    }
}

uint32_t DataManager::getNewSleepPeriod(uint8_t room_id) const {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            uint32_t new_sleep_period_ms = rooms[room_id].sensor.new_sleep_period_ms;
        xSemaphoreGive(sensorMutex);
        return new_sleep_period_ms;
    } else{
        return 0;
    }
}

bool DataManager::isPendingUpdate(uint8_t room_id, NodeType node_type) const {
    if (roomIdIsValid(room_id)){
        bool pending;
        if (node_type == NodeType::SENSOR){
            xSemaphoreTake(sensorMutex, portMAX_DELAY);
                pending = rooms[room_id].sensor.pending_update;
            xSemaphoreGive(sensorMutex);
        } else{
            xSemaphoreTake(controlMutex, portMAX_DELAY);
                pending = rooms[room_id].control.pending_update;
            xSemaphoreGive(controlMutex);
        }
        
        return pending;
    } else {
        return false;
    }
}

RoomData DataManager::getRoomData(uint8_t room_id) const {
    RoomData data;
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            data = rooms[room_id];
        xSemaphoreGive(sensorMutex);
    }
    return data;
}

bool DataManager::getMacAddr(uint8_t room_id, NodeType node_type, uint8_t* out_mac_addr) const {
    if (roomIdIsValid(room_id) && out_mac_addr != nullptr){
        if (node_type == NodeType::SENSOR){
            xSemaphoreTake(sensorMutex, portMAX_DELAY);
                if (!rooms[room_id].sensor.registered){
                    LOG_WARNING("Sensor MAC address is not registered.");
                    xSemaphoreGive(sensorMutex);
                    return false;
                }
                memcpy(out_mac_addr, rooms[room_id].sensor.mac_addr, MAC_ADDRESS_LENGTH);
            xSemaphoreGive(sensorMutex);
        } else {
            xSemaphoreTake(controlMutex, portMAX_DELAY);
                if (!rooms[room_id].control.registered){
                    LOG_WARNING("Control MAC address is not registered.");
                    xSemaphoreGive(controlMutex);
                    return false;
                }
                memcpy(out_mac_addr, rooms[room_id].control.mac_addr, MAC_ADDRESS_LENGTH);
            xSemaphoreGive(controlMutex);
        }
        return true;
    }
    LOG_WARNING("Invalid room_id or out_mac_addr is nullptr");
    return false;
}

void DataManager::sensorSetup(uint8_t room_id, const uint8_t* mac_addr, uint32_t sleep_period_ms){
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            memcpy(rooms[room_id].sensor.mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
            rooms[room_id].sensor.sleep_period_ms = sleep_period_ms;
            rooms[room_id].sensor.new_sleep_period_ms = sleep_period_ms;
            rooms[room_id].sensor.pending_update = false; // No pending update initially
            rooms[room_id].sensor.registered = true; // Register sensor
        xSemaphoreGive(sensorMutex);
    }
}

void DataManager::setNewSchedule(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min) {
    if (roomIdIsValid(room_id)) {
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            rooms[room_id].control.pending_update = true;
            rooms[room_id].control.cold.hour = cold_hour;
            rooms[room_id].control.cold.min = cold_min;
            rooms[room_id].control.warm.hour = warm_hour;
            rooms[room_id].control.warm.min = warm_min;
        xSemaphoreGive(controlMutex);
    }
}

void DataManager::scheduleWasUpdated(uint8_t room_id) {
    if (roomIdIsValid(room_id)) {
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            rooms[room_id].control.pending_update = false;
        xSemaphoreGive(controlMutex);
        LOG_INFO("Schedule was successfully updated");
    }
}

void DataManager::sleepPeriodWasUpdated(uint8_t room_id){
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            rooms[room_id].sensor.sleep_period_ms = rooms[room_id].sensor.new_sleep_period_ms;
            rooms[room_id].sensor.pending_update = false;
        xSemaphoreGive(sensorMutex);
        LOG_INFO("Sleep Period was successfully updated");
    }
}

bool DataManager::roomIdIsValid(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS){
        LOG_WARNING("The Room ID is not valid.");
        return false;
    }
    return true;
}

uint8_t DataManager::getId(uint8_t* mac_addr) const {
    xSemaphoreTake(controlMutex, portMAX_DELAY);
    xSemaphoreTake(sensorMutex, portMAX_DELAY);
        for (uint8_t i = 0; i < NUM_ROOMS; i++){
            if (rooms[i].sensor.registered){
                if(memcmp(rooms[i].sensor.mac_addr, mac_addr, MAC_ADDRESS_LENGTH) == 0){
                    // Release semaphores before returning
                    xSemaphoreGive(sensorMutex); 
                    xSemaphoreGive(controlMutex);
                    return i;
                }
            }
            if (rooms[i].control.registered){
                if(memcmp(rooms[i].control.mac_addr, mac_addr, MAC_ADDRESS_LENGTH) == 0){
                    // Release semaphore before returning
                    xSemaphoreGive(sensorMutex); 
                    xSemaphoreGive(controlMutex);
                    return i;
                }
            }
        }
    xSemaphoreGive(sensorMutex);
    xSemaphoreGive(controlMutex);
    LOG_WARNING("MAC address is not registered.");
    return ID_NOT_VALID;
}

void DataManager::controlSetup(uint8_t room_id, const uint8_t* mac_addr, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min) {
    if (roomIdIsValid(room_id)) {
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            memcpy(rooms[room_id].control.mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
            rooms[room_id].control.warm.hour = warm_hour;
            rooms[room_id].control.warm.min = warm_min;
            rooms[room_id].control.cold.hour = cold_hour;
            rooms[room_id].control.cold.min = cold_min;
            rooms[room_id].control.registered = true; 
            rooms[room_id].control.pending_update = false; // No pending update initially
            rooms[room_id].control.latest_heartbeat = millis();
        xSemaphoreGive(controlMutex);

        LOG_INFO("Control setup for room %u: Warm=%02u:%02u, Cold=%02u:%02u", room_id, warm_hour, warm_min, cold_hour, cold_min);
    }
}

void DataManager::updateHeartbeat(uint8_t room_id){
    if (roomIdIsValid(room_id)) {
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            rooms[room_id].control.latest_heartbeat = millis();
        xSemaphoreGive(controlMutex);

        LOG_INFO("Heartbeat from room with ID %u was successfully updated", room_id);
    }
}

uint32_t DataManager::getLatestHeartbeat(uint8_t room_id){
    if (roomIdIsValid(room_id)){
        uint32_t latest_heartbeat;
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            latest_heartbeat = rooms[room_id].control.latest_heartbeat;
        xSemaphoreGive(controlMutex);
        return latest_heartbeat;
    }
    return 0;
}

bool DataManager::isRegistered(uint8_t room_id, NodeType type){
    bool value = false;
    xSemaphoreTake(controlMutex, portMAX_DELAY);
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            if (type == NodeType::NONE){
                value = rooms[room_id].isRegistered();
            } else if (type == NodeType::ROOM){
                value = rooms[room_id].control.registered;
            } else if (type == NodeType::SENSOR){
                value = rooms[room_id].sensor.registered;
            } else {
                LOG_WARNING("Unknown NodeType provided to isRegistered.");
            }
        xSemaphoreGive(sensorMutex);
    xSemaphoreGive(controlMutex);

    return value;
}

void DataManager::unregisterNode(uint8_t room_id, NodeType type){
    if (type == NodeType::ROOM){
        xSemaphoreTake(controlMutex, portMAX_DELAY);
            rooms[room_id].control.registered = false; 
        xSemaphoreGive(controlMutex);
    } else if (type == NodeType::SENSOR){
        xSemaphoreTake(sensorMutex, portMAX_DELAY);
            rooms[room_id].sensor.registered = false; 
        xSemaphoreGive(sensorMutex);
    }
}