/**
 * @file DataManager.cpp
 * @brief Implementation of DataManager class for managing sensor data storage and retrieval
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/DataManager.h"

DataManager::DataManager() {
    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        Serial.println("Failed to create dataMutex semaphore!");
    } else {
        Serial.println("dataMutex semaphore created successfully.");
    }
}

void DataManager::addSensorData(uint8_t room_id, float temperature, float humidity, time_t timestamp) {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            RoomData& room = rooms[room_id];
            uint16_t idx = room.index;
            room.temperature[idx] = temperature;
            room.humidity[idx] = humidity;
            room.timestamps[idx] = timestamp;
            room.valid_data_points++;
            if (room.valid_data_points > MAX_DATA_POINTS) {
                room.valid_data_points = MAX_DATA_POINTS;
            }
            // Update index
            room.index = (idx + 1) % MAX_DATA_POINTS;
        xSemaphoreGive(dataMutex);
    }
}

void DataManager::setNewSleepPeriod(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            if (rooms[room_id].sleep_period_ms != new_sleep_period_ms){
                rooms[room_id].new_sleep_period_ms = new_sleep_period_ms;
                rooms[room_id].pending_update = true;
            }
        xSemaphoreGive(dataMutex);
    }
}

uint32_t DataManager::getNewSleepPeriod(uint8_t room_id) const {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            uint32_t new_sleep_period_ms = rooms[room_id].new_sleep_period_ms;
        xSemaphoreGive(dataMutex);
        return new_sleep_period_ms;
    } else{
        return 0;
    }
}

bool DataManager::isPendingUpdate(uint8_t room_id) const {
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            bool pending = rooms[room_id].pending_update;
        xSemaphoreGive(dataMutex);
        return pending;
    } else {
        return false;
    }
}

RoomData DataManager::getRoomData(uint8_t room_id) const {
    RoomData data;
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            data = rooms[room_id];
        xSemaphoreGive(dataMutex);
    }
    return data;
}

bool DataManager::getMacAddr(uint8_t room_id, uint8_t* out_mac_addr) const {
    if (roomIdIsValid(room_id) && out_mac_addr != nullptr){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            memcpy(out_mac_addr, rooms[room_id].mac_addr, MAC_ADDRESS_LENGTH);
        xSemaphoreGive(dataMutex);

        uint8_t mac_not_valid[MAC_ADDRESS_LENGTH] = {0};
        if (memcmp(out_mac_addr, mac_not_valid, MAC_ADDRESS_LENGTH) == 0){
            Serial.println("MAC address is not defined");
            return false;
        }
        return true;
    }
    Serial.println("Invalid room_id or out_mac_addr is nullptr");
    return false;
}

void DataManager::sensorSetup(uint8_t room_id, const uint8_t* mac_addr, uint32_t sleep_period_ms){
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            memcpy(rooms[room_id].mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
            rooms[room_id].sleep_period_ms = sleep_period_ms;
            rooms[room_id].new_sleep_period_ms = sleep_period_ms;
            rooms[room_id].pending_update = false; // No pending update initially
            rooms[room_id].registered = true; //Register room
        xSemaphoreGive(dataMutex);
    }
}

void DataManager::sleepPeriodWasUpdated(uint8_t room_id){
    if (roomIdIsValid(room_id)){
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            rooms[room_id].sleep_period_ms = rooms[room_id].new_sleep_period_ms;
            rooms[room_id].pending_update = false;
        xSemaphoreGive(dataMutex);
        Serial.println("Sleep Period was successfully updated");
    }
}

bool DataManager::roomIdIsValid(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS){
        Serial.println("The Room ID is not valid.");
        return false;
    }
    return true;
}

uint8_t DataManager::getId(uint8_t* mac_addr) const {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        for (uint8_t i = 0; i < NUM_ROOMS; i++){
           if (rooms[i].registered){
                if(memcmp(rooms[i].mac_addr, mac_addr, MAC_ADDRESS_LENGTH) == 0){
                    xSemaphoreGive(dataMutex); // Release semaphore before returning
                    return i;
                }
           }
        }
    xSemaphoreGive(dataMutex);
    Serial.println("Mac address is not registered.");
    return ID_NOT_VALID;
}
