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

const RoomData& DataManager::getRoomData(uint8_t room_id) const {
    // Ensure thread-safe access
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    const RoomData& data = rooms[room_id];
    xSemaphoreGive(dataMutex);
    return data;
}
