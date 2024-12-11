/**
 * @file ESPNowHandler.h
 * @brief Declaration of ESPNowHandler class for ESP-NOW communication from SensorNode to MasterDevice
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */
#pragma once

#include "Common/CommunicationsBase.h"
#include <freertos/semphr.h>
#include "PowerManager.h"

class ESPNowHandler : public CommunicationsBase {
public:
    // Constructs with reference to PowerManager for sleep updates
    ESPNowHandler(PowerManager& powerManager);

    // Initializes ESP-NOW and attempts to communicate with master
    bool initializeESPNOW(const uint8_t* master_mac_address, const uint8_t channel);

    // Sends a message to the master
    void sendMsg(const uint8_t* data, size_t size);

    // Waits for an ACK of the specified type within a timeout
    bool waitForAck(MessageType expected_ack, unsigned long timeout_ms);

private:
    // Handles incoming data; if ACK or new sleep period, process accordingly
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;

    PowerManager& powerManager;

    volatile bool ack_received;
    MessageType last_acked_msg;
    SemaphoreHandle_t ackSemaphore; // Signals when ACK is received

    static ESPNowHandler* instance;
};