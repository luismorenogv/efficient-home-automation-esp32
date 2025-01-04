/**
 * @file RoomNodeCommunications.h
 * @brief Handles communication for RoomNode, waiting for ACK messages from Master.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include "Common/CommunicationsBase.h"
#include <freertos/semphr.h>

class RoomCommunications : public CommunicationsBase {
public:
    RoomCommunications(SemaphoreHandle_t* radioMutex);

    // Waits for an ACK of a given type or times out
    bool waitForAck(MessageType expected_ack, unsigned long timeout_ms);

    // Called when an ACK is received, signals semaphore if it matches
    void ackReceived(uint8_t *mac_addr, MessageType acked_msg);

    // Sends a message to the master (assumes one peer)
    bool sendMsg(const uint8_t *data, size_t size);

private:
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;
    int findPeerIndex(uint8_t* mac_addr);

    volatile bool ack_received;
    MessageType last_acked_msg;
    SemaphoreHandle_t ackSemaphore;
    SemaphoreHandle_t* radioMutex;
};