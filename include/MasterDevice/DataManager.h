/**
 * @file DataManager.h
 * @brief Declaration of DataManager class for managing sensor data storage and retrieval
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#pragma once

#include <Arduino.h>
#include <time.h>
#include "common.h"
#include "MasterConfig.h"
#include <freertos/semphr.h>

struct RoomData {
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    uint16_t index;
    uint16_t valid_data_points;

    RoomData() : index(0), valid_data_points(0) {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
    }
};

class DataManager {
public:
    DataManager();
    void addData(uint8_t room_id, float temperature, float humidity, time_t timestamp);
    const RoomData& getRoomData(uint8_t room_id) const;

private:
    RoomData rooms[NUM_ROOMS];
    mutable SemaphoreHandle_t dataMutex;
};
