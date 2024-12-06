/**
 * @file RoomNode.h
 * @brief Declaration of RoomNode class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 2, 2024
 */
#pragma once

#include <Arduino.h>
#include "esp_wifi.h"
#include "Common/common.h"
#include "config.h"
#include "Common/NTPClient.h"
#include "RoomCommunications.h"
#include "DataManager.h"


class RoomNode {
public:
    RoomNode(uint8_t room_id);
    void initialize();
    bool joinNetwork();
    void run();

private:
    NTPClient ntpClient;
    RoomCommunications communications;
    DataManager dataManager;
    static RoomNode* instance;

    uint8_t wifi_channel;
    uint8_t room_id;

    QueueHandle_t espnowQueue;
    QueueHandle_t presenceQueue;

    TaskHandle_t espnowTaskHandle;
    TaskHandle_t lightsControlTaskHandle;
    TaskHandle_t presenceTaskHandle;
    TaskHandle_t NTPSyncTaskHandle;

    static void espnowTask(void* pvParameter);
    static void lightsControlTask(void* pvParameter);
    static void presenceTask(void* pvParameter);
    static void NTPSyncTask(void* pvParameter);

};