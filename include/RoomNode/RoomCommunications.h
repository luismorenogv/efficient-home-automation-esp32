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
    bool waitForAck(MessageType expected_ack, unsigned long timeout_ms);
    void ackReceived(uint8_t *mac_addr, MessageType acked_msg);
    void sendMsg(const uint8_t *data, size_t size);

private:
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;
    int findPeerIndex(uint8_t* mac_addr);

    volatile bool ack_received;
    MessageType last_acked_msg;
    SemaphoreHandle_t ackSemaphore;
};
