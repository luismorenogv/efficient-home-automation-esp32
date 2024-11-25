/**
 * @file DataManager.h
 * @brief Declaration of DataManager class for managing sensor data storage and retrieval
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include <Arduino.h>
#include <time.h>
#include "common.h"
#include "config.h"
#include <freertos/semphr.h>

struct RoomData {
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    uint32_t sleep_period_ms;
    uint16_t index;
    uint16_t valid_data_points;
    bool pending_update;
    uint32_t new_sleep_period_ms; 
    bool registered;

    RoomData() : sleep_period_ms(DEFAULT_SLEEP_DURATION), index(0), valid_data_points(0),
                 pending_update(false), new_sleep_period_ms(DEFAULT_SLEEP_DURATION), registered(false) {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
        memset(mac_addr, 0, sizeof(mac_addr));
    }
};

class DataManager {
public:
    DataManager();
    // Delete copy constructor and copy assignment operator
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    void addSensorData(uint8_t room_id, float temperature, float humidity, time_t timestamp);
    RoomData getRoomData(uint8_t room_id) const;
    void sensorSetup(uint8_t room_id, const uint8_t* mac_addr, uint32_t sleep_period_ms);
    bool getMacAddr(uint8_t room_id, uint8_t* out_mac_addr) const; 
    void setNewSleepPeriod(uint8_t room_id, uint32_t new_sleep_period_ms);
    uint32_t getNewSleepPeriod(uint8_t room_id) const;
    bool isPendingUpdate(uint8_t room_id) const;
    void sleepPeriodWasUpdated(uint8_t room_id);
    uint8_t getId(uint8_t* mac_addr) const;
private:
    RoomData rooms[NUM_ROOMS];
    mutable SemaphoreHandle_t dataMutex;
    bool roomIdIsValid(uint8_t room_id) const;
};
