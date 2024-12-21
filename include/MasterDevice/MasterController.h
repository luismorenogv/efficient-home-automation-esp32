/**
 * @file MasterController.h
 * @brief Declaration of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include "config.h"
#include "Common/mac_addrs.h"
#include "MasterCommunications.h"
#include "Common/NTPClient.h"
#include "DataManager.h"
#include "WebServer.h"
#include "WebSockets.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <Arduino.h>

// Structure to track pending updates for rooms
struct PendingUpdate {
    uint8_t room_id;            // Room identifier
    uint8_t attempts;           // Number of resend attempts
    uint32_t lastAttemptMillis; // Timestamp of the last attempt

    PendingUpdate() 
        : room_id(0), attempts(0), lastAttemptMillis(0) {}
};

// Class to coordinate communication, data management, and web interfaces
class MasterController {
public:
    MasterController();
    void initialize();

private:
    // Retry configurations
    static constexpr uint8_t MAX_RETRIES = 3;           // Maximum number of resend attempts
    static constexpr uint32_t RETRY_INTERVAL_MS = 5000; // Interval between retries in milliseconds

    PendingUpdate pendingSleepUpdate[NUM_ROOMS];       // Tracks sleep period updates
    PendingUpdate pendingScheduleUpdate[NUM_ROOMS];    // Tracks schedule updates

    // Module instances
    MasterCommunications communications;  // Handles communication protocols
    NTPClient ntpClient;                  // Manages NTP synchronization
    DataManager dataManager;              // Manages sensor and control data
    WebServer webServer;                  // Hosts the web interface
    WebSockets webSockets;                // Manages WebSocket communications

    // FreeRTOS task handles
    TaskHandle_t espnowTaskHandle;      // Handle for ESP-NOW Task
    TaskHandle_t webServerTaskHandle;   // Handle for Web Server Task
    QueueHandle_t espnowQueue;          // Queue for incoming ESP-NOW messages
    TaskHandle_t ntpSyncTaskHandle;     // Handle for NTP Sync Task 

    // Sets the master to sleep for the next 30min
    void tryLater();

    // Callback functions for WebSockets
    static void sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms);
    static void scheduleChangedCallback(uint8_t room_id, uint8_t warm_hour, uint8_t warm_min, 
                                        uint8_t cold_hour, uint8_t cold_min);

    // Task functions
    static void espnowTask(void* pvParameter);
    static void webServerTask(void* pvParameter);
    static void ntpSyncTask(void* pvParameter);
    static void updateCheckTask(void* pvParameter);

    // Checks and resends pending updates
    void checkAndResendUpdates();

    // Checks if latest heartbeat is valid for each room
    void checkHeartbeats();

    // Checks if expected new SensorNode message has arrived in time or expired
    void checkSensorNodes();
};