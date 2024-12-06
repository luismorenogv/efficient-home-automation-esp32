/**
 * @file DataManager.h
 * @brief Manages sensor and room control data.
 * 
 * ControlData now stores daily switch times as hours and minutes.
 * This avoids timestamps and focuses on the daily schedule.
 * 
 * @author Luis Moreno
 * @date Dec 6, 2024
 */

#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>
#include "Common/common.h"
#include "config.h"

struct SensorData {
    bool registered;
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    uint32_t sleep_period_ms;
    uint16_t index;
    uint16_t valid_data_points;
    bool pending_update;
    uint32_t new_sleep_period_ms;

    SensorData() : registered(false), sleep_period_ms(DEFAULT_SLEEP_DURATION), index(0),
                   valid_data_points(0), pending_update(false), new_sleep_period_ms(DEFAULT_SLEEP_DURATION) {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
        memset(mac_addr, 0, sizeof(mac_addr));
    }
};

struct ControlData {
    bool registered;
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    uint8_t warm_hour;
    uint8_t warm_min;
    uint8_t cold_hour;
    uint8_t cold_min;
    bool pending_update;

    ControlData() : registered(false), warm_hour(0), warm_min(0),
                    cold_hour(0), cold_min(0), pending_update(false) {
        memset(mac_addr, 0, sizeof(mac_addr));
    }
};

struct RoomData {
    SensorData sensor;
    ControlData control;

    // Convenient flag to determine if any node is registered for this room
    bool isRegistered() const {
        return sensor.registered || control.registered;
    }
};

class DataManager {
public:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    void addSensorData(uint8_t room_id, float temperature, float humidity, time_t timestamp);
    RoomData getRoomData(uint8_t room_id) const;
    void sensorSetup(uint8_t room_id, const uint8_t* mac_addr, uint32_t sleep_period_ms);

    void controlSetup(uint8_t room_id, const uint8_t* mac_addr, uint8_t warm_hour, uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min);

    bool getMacAddr(uint8_t room_id, NodeType node_type, uint8_t* out_mac_addr) const; 
    void setNewSleepPeriod(uint8_t room_id, uint32_t new_sleep_period_ms);
    uint32_t getNewSleepPeriod(uint8_t room_id) const;
    bool isPendingUpdate(uint8_t room_id, NodeType node_type) const;
    void sleepPeriodWasUpdated(uint8_t room_id);
    uint8_t getId(uint8_t* mac_addr) const;

private:
    RoomData rooms[NUM_ROOMS];
    mutable SemaphoreHandle_t sensorMutex;
    mutable SemaphoreHandle_t controlMutex;
    bool roomIdIsValid(uint8_t room_id) const;
};

