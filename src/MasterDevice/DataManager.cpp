/**
 * @file DataManager.cpp
 * @brief Implementation of DataManager class for managing sensor data storage and retrieval
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include "MasterDevice/DataManager.h"

DataManager::DataManager() {
    dataMutex = xSemaphoreCreateMutex();
}

void DataManager::addData(uint8_t room_id, float temperature, float humidity, time_t timestamp) {
    if (room_id >= NUM_ROOMS) return;
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

void DataManager::setWakeInterval(uint8_t room_id, uint32_t wake_interval_ms) {
    if (room_id >= NUM_ROOMS) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        rooms[room_id].wake_interval_ms = wake_interval_ms;
    xSemaphoreGive(dataMutex);
}

uint32_t DataManager::getWakeInterval(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS) return 0;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        uint32_t interval = rooms[room_id].wake_interval_ms;
    xSemaphoreGive(dataMutex);
    return interval;
}

const RoomData& DataManager::getRoomData(uint8_t room_id) const {
    // Ensure thread-safe access
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    const RoomData& data = rooms[room_id];
    xSemaphoreGive(dataMutex);
    return data;
}
void DataManager::setPollingPeriodUpdate(uint8_t room_id, uint32_t new_period_ms) {
    if (room_id >= NUM_ROOMS) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        rooms[room_id].wake_up_interval_updated = true;
        rooms[room_id].new_wake_up_interval = new_period_ms;
    xSemaphoreGive(dataMutex);
}

bool DataManager::isPollingPeriodUpdated(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS) return false;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        bool updated = rooms[room_id].wake_up_interval_updated;
    xSemaphoreGive(dataMutex);
    return updated;
}

uint32_t DataManager::getNewPollingPeriod(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS) return 0;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        uint32_t new_period = rooms[room_id].new_wake_up_interval;
    xSemaphoreGive(dataMutex);
    return new_period;
}

const uint8_t* DataManager::getMacAddr(uint8_t room_id) const {
    if (room_id >= NUM_ROOMS) return 0;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
        const uint8_t* mac_addr = rooms[room_id].mac_addr;
    xSemaphoreGive(dataMutex);
    return mac_addr;
}

void DataManager::setMacAddr(uint8_t room_id, const uint8_t* mac_addr){
    if (room_id >= NUM_ROOMS) return;
        xSemaphoreTake(dataMutex, portMAX_DELAY);
            memcpy(rooms[room_id].mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
        xSemaphoreGive(dataMutex);
}