/**
 * @file ESPNowHandler.h
 * @brief Declaration of ESPNowHandler class for ESP-NOW communication with MasterDevice
 *
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#pragma once

#include "Common/CommunicationsBase.h"
#include <freertos/semphr.h>
#include "PowerManager.h"

class ESPNowHandler : public CommunicationsBase {
public:
    ESPNowHandler(PowerManager& powerManager);
    bool initializeESPNOW(const uint8_t *master_mac_address, const uint8_t channel);
    void sendMsg(const uint8_t* data, size_t size);
    bool waitForAck(MessageType expected_ack, unsigned long timeout_ms);

private:
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;

    PowerManager& powerManager;

    volatile bool ack_received;
    MessageType last_acked_msg;

    SemaphoreHandle_t ackSemaphore;

    // Static instance pointer
    static ESPNowHandler* instance;
};
