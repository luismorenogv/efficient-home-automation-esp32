/**
 * @file RoomNodeCommunications.h
 * @brief Declaration of RoomNodeCommunications class for RoomNode communication
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#pragma once

#include "Common/CommunicationsBase.h"
#include <freertos/semphr.h>

class RoomCommunications : public CommunicationsBase {
public:
    RoomCommunications();
    bool waitForAck(uint8_t* mac_addr, MessageType expected_ack, unsigned long timeout_ms);
    void sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size);
    SemaphoreHandle_t getSemphr(uint8_t* mac_address);

private:
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;
    int findPeerIndex(uint8_t* mac_addr);

    // Structure to manage ACKs per peer
    struct AckInfo {
        volatile bool ack_received;
        MessageType last_acked_msg;
        SemaphoreHandle_t ackSemaphore;
    };

    AckInfo ackInfo[MAX_PEERS];
};
