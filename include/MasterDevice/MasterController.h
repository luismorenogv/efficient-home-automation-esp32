/**
 * @file MasterController.h
 * @brief Declaration of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include "config.h"
#include "Common/mac_addrs.h"
#include "Common/Communications.h"
#include "Common/NTPClient.h"
#include "DataManager.h"
#include "WebServer.h"
#include "WebSockets.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <Arduino.h>

class MasterController {
public:
    MasterController();
    void initialize();

private:
    Communications communications;
    NTPClient ntpClient;
    DataManager dataManager;
    WebServer webServer;
    WebSockets webSockets;

    TaskHandle_t espnowTaskHandle;
    TaskHandle_t webServerTaskHandle;
    QueueHandle_t espnowQueue;

    static void sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms);
    static void espnowTask(void* pvParameter);
    static void webServerTask(void* pvParameter);
};
