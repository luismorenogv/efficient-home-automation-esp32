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
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    uint32_t wake_interval_ms;         // Current wake interval
    uint16_t index;
    uint16_t valid_data_points;
    bool wake_up_interval_updated;     // Flag to indicate polling period update
    uint32_t new_wake_up_interval;       // New pollin period to be set

    RoomData() : wake_interval_ms(0), index(0), valid_data_points(0),
                 wake_up_interval_updated(true), new_wake_up_interval(0)  {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
        memset(mac_addr, 0, sizeof(mac_addr));
    }
};

class DataManager {
public:
    DataManager();
    void addData(uint8_t room_id, float temperature, float humidity, time_t timestamp);
    const RoomData& getRoomData(uint8_t room_id) const;
    void setMacAddr(uint8_t room_id, const uint8_t* mac_addr);
    const uint8_t* getMacAddr(uint8_t room_id) const;   
    void setWakeInterval(uint8_t room_id, uint32_t wake_interval_ms);
    uint32_t getWakeInterval(uint8_t room_id) const;   
    void setPollingPeriodUpdate(uint8_t room_id, uint32_t new_period_ms);
    bool isPollingPeriodUpdated(uint8_t room_id) const;
    uint32_t getNewPollingPeriod(uint8_t room_id) const;         

private:
    RoomData rooms[NUM_ROOMS];
    mutable SemaphoreHandle_t dataMutex;
};
