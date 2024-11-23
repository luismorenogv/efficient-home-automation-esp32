/**
 * @file ESPNowHandler.h
 * @brief Declaration of ESPNowHandler class for ESP-NOW communication with MasterDevice
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include "SensorConfig.h"
#include "PowerManager.h"
#include "common.h"

class ESPNowHandler {
public:
    ESPNowHandler(PowerManager& powerManager);
    bool initializeESPNOW(const uint8_t* master_mac_address);
    void sendMsg(const uint8_t* data, size_t size);
    bool waitForAck(MessageType expected_ack, unsigned long timeout_ms);

private:
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len);
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len);

    static ESPNowHandler* instance;

    PowerManager& powerManager;

    esp_now_peer_info_t master_peer;
    volatile bool ack_received;
    MessageType last_acked_msg;
};
