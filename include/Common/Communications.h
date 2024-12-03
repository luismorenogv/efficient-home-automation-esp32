/**
 * @file Communications.h
 * @brief Declaration of Communications class for ESP-NOW communication with SensorNodes
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include "common.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

class Communications {
public:
    Communications();
    void initializeWifi();
    bool initializeESPNOW();
    bool registerPeer(uint8_t* master_mac_address);
    void sendAck(const uint8_t* mac_addr, MessageType acked_msg);
    void sendMsg(const uint8_t* mac_addr, const uint8_t* data, size_t size);
    void setQueue(QueueHandle_t queue);

private:
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len);
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len);

    static Communications* instance;
    QueueHandle_t dataQueue;
};
