/**
 * @file RoomNodeCommunications.cpp
 * @brief Implementation of RoomNodeCommunications class for RoomNode communication
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#include "RoomNode/RoomCommunications.h"

RoomCommunications::RoomCommunications()
    : ack_received(false), last_acked_msg(MessageType::ACK), CommunicationsBase(){
    instance = this;
    ackSemaphore = xSemaphoreCreateBinary();
}

bool RoomCommunications::waitForAck(uint8_t* mac_addr, MessageType expected_ack, unsigned long timeout_ms) {
    // Set expected ACK
    ack_received = false;
    last_acked_msg = expected_ack;

    // Wait for the ACK semaphore
    if (xSemaphoreTake(ackSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ack_received;
    }

    Serial.println("ACK wait timed out.");
    return false;
}

void RoomCommunications::sendMsg(const uint8_t* data, size_t size) {
    // Assume only one peer (master)
    if (numPeers > 0) {
        CommunicationsBase::sendMsg(peers[0].mac_addr, data, size);
    } else {
        Serial.println("No peers registered.");
    }
}

SemaphoreHandle_t RoomCommunications::getSemphr(uint8_t* mac_addr){
    return ackSemaphore;
}