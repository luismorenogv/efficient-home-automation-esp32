/**
 * @file RoomNode.h
 * @brief Coordinates modules in the RoomNode: presence, lights, AC, and communication.
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */
#pragma once

#include <Arduino.h>
#include "esp_wifi.h"
#include "Common/common.h"
#include "config.h"
#include "Common/NTPClient.h"
#include "RoomCommunications.h"
#include "LD2410.h"
#include "Lights.h"
#include "Common/secrets.h"
#include "AirConditioner.h"

class RoomNode {
public:
    RoomNode(uint8_t room_id);

    // Initializes node components and network
    void initialize();

    // Attempts to join the master network
    bool joinNetwork();

    // Creates tasks and starts node operation
    void run();

private:
    const uint32_t HEARTBEAT_TIMEOUT = 2000;
    NTPClient ntpClient;
    RoomCommunications communications;
    LD2410 presenceSensor;
    Lights lights;
    AirConditioner airConditioner;
    static RoomNode* instance;

    uint8_t wifi_channel;
    uint8_t room_id;
    bool connected;
    bool user_stop;
    Time cold;
    Time warm;

    QueueHandle_t espnowQueue;
    QueueHandle_t presenceQueue;
    QueueHandle_t lightsToggleQueue;

    TaskHandle_t espnowTaskHandle;
    TaskHandle_t lightsControlTaskHandle;
    TaskHandle_t presenceTaskHandle;
    TaskHandle_t NTPSyncTaskHandle;
    TaskHandle_t HeartBeatTaskHandle;
    TaskHandle_t LightsToggleTaskHandle;

    SemaphoreHandle_t radioMutex;

    // Task functions
    static void espnowTask(void* pvParameter);
    static void lightsControlTask(void* pvParameter);
    static void presenceTask(void* pvParameter);
    static void NTPSyncTask(void* pvParameter);
    static void heartbeatTask(void* pvParameter);
    static void lightsToggleTask(void* pvParameter);

    // If initialization fails, sleeps to retry later
    void tryLater();
};