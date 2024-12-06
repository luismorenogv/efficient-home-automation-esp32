/**
 * @file RoomNode.h
 * @brief Declaration of RoomNode class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 2, 2024
 */
#pragma once

#include <Arduino.h>
#include "Common/common.h"
#include "config.h"
#include "Common/NTPClient.h"
#include "RoomCommunications.h"
#include "DataManager.h"


class RoomNode {
public:
    RoomNode();
    void initialize();

private:
    NTPClient ntpClient;
    RoomCommunications communications;
    DataManager dataManager;
    static RoomNode* instance;

    QueueHandle_t espnowQueue;
    QueueHandle_t presenceQueue;

    TaskHandle_t espnowTaskHandle;
    TaskHandle_t lightsControlTaskHandle;
    TaskHandle_t presenceTaskHandle;

    static void espnowTask(void* pvParameter);
    static void lightsControlTask(void* pvParameter);
    static void presenceTask(void* pvParameter);

};