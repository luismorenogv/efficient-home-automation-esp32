/**
 * @file DataManager.h
 * @brief Manages sensor and room control data.

 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <Arduino.h>
#include <freertos/semphr.h>
#include "Common/common.h"
#include "config.h"

// Structure to hold sensor-related data for a room
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

    SensorData() 
        : registered(false), sleep_period_ms(DEFAULT_SLEEP_DURATION), index(0),
          valid_data_points(0), pending_update(false), new_sleep_period_ms(DEFAULT_SLEEP_DURATION) 
    {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
        memset(mac_addr, 0, sizeof(mac_addr));
    }
};

// Structure to hold control-related data for a room
struct ControlData {
    bool registered;
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    Time cold;
    Time warm;
    bool pending_update;
    uint32_t latest_heartbeat;

    ControlData() 
        : registered(false), pending_update(false), latest_heartbeat(millis()) 
    {
        memset(mac_addr, 0, sizeof(mac_addr));
        memset(&cold, 0, sizeof(cold));
        memset(&warm, 0, sizeof(warm));
    }
};

// Combines sensor and control data for a room
struct RoomData {
    SensorData sensor;
    ControlData control;

    bool isRegistered() const {
        return sensor.registered || control.registered;
    }
};

// Manages data for all rooms, ensuring thread-safe operations
class DataManager {
public:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    // Adds new sensor data for a specific room
    void addSensorData(uint8_t room_id, float temperature, float humidity, time_t timestamp);
    
    // Retrieves data for a specific room
    RoomData getRoomData(uint8_t room_id) const;
    
    // Sets up sensor data for a room
    void sensorSetup(uint8_t room_id, const uint8_t* mac_addr, uint32_t sleep_period_ms);

    // Sets up control data for a room
    void controlSetup(uint8_t room_id, const uint8_t* mac_addr, 
                     uint8_t warm_hour, uint8_t warm_min, 
                     uint8_t cold_hour, uint8_t cold_min);
    
    // Sets a new sleep period for a room
    void setNewSleepPeriod(uint8_t room_id, uint32_t new_sleep_period_ms);
    
    // Retrieves the new sleep period for a room
    uint32_t getNewSleepPeriod(uint8_t room_id) const;
    
    // Sets a new schedule for a room
    void setNewSchedule(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, 
                       uint8_t cold_hour, uint8_t cold_min);
    
    // Marks that the schedule was successfully updated
    void scheduleWasUpdated(uint8_t room_id);
    
    // Checks if there is a pending update for a room
    bool isPendingUpdate(uint8_t room_id, NodeType node_type) const;
    
    // Marks that the sleep period was successfully updated
    void sleepPeriodWasUpdated(uint8_t room_id);
    
    // Retrieves the MAC address for a specific room and node type
    bool getMacAddr(uint8_t room_id, NodeType node_type, uint8_t* out_mac_addr) const; 
    
    // Retrieves the room ID based on MAC address
    uint8_t getId(uint8_t* mac_addr) const;

    // Updates latest room heartbeat
    void updateHeartbeat(uint8_t room_id);

    // Returns latest heartbeat in ms from room_id
    uint32_t getLatestHeartbeat(uint8_t room_id);

    // Returns is a sensorNode, roomNode or either of them is registered
    bool isRegistered(uint8_t room_id, NodeType type = NodeType::NONE);

    // Unregisters roomNode or sensorNode
    void unregisterNode(uint8_t room_id, NodeType type);

private:
    RoomData rooms[NUM_ROOMS];
    mutable SemaphoreHandle_t sensorMutex;
    mutable SemaphoreHandle_t controlMutex;

    // Validates the room ID
    bool roomIdIsValid(uint8_t room_id) const;
};
