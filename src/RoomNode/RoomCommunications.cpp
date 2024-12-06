/**
 * @file RoomNodeCommunications.cpp
 * @brief Implementation of RoomNodeCommunications class for RoomNode communication
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#include "RoomNode/RoomCommunications.h"

RoomCommunications::RoomCommunications()
    : CommunicationsBase(){
    instance = this;
    // Initialize AckInfo for all peers
    for (int i = 0; i < MAX_PEERS; ++i) {
        ackInfo[i].ack_received = false;
        ackInfo[i].last_acked_msg = MessageType::ACK;
        ackInfo[i].ackSemaphore = xSemaphoreCreateBinary();
    }
}

bool RoomCommunications::waitForAck(uint8_t* mac_addr, MessageType expected_ack, unsigned long timeout_ms) {
    int peerIndex = findPeerIndex(mac_addr);
    if (peerIndex < 0){
        Serial.println("Mac address hasn't been registered as peer.");
        return false;
    }
    // Set expected ACK
    ackInfo[peerIndex].ack_received = false;
    ackInfo[peerIndex].last_acked_msg = expected_ack;

    // Wait for the ACK semaphore
    if (xSemaphoreTake(ackInfo[peerIndex].ackSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ackInfo[peerIndex].ack_received;
    }

    Serial.println("ACK wait timed out.");
    return false;
}

SemaphoreHandle_t RoomCommunications::getSemphr(uint8_t* mac_addr){
    int peerIndex = findPeerIndex(mac_addr);
    if (peerIndex < 0){
        Serial.println("Mac address hasn't been registered as peer.");
        return nullptr;
    }
    return ackInfo[peerIndex].ackSemaphore;
}

void RoomCommunications::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    // Enqueue the message for processing in the FreeRTOS task
    IncomingMsg incoming_msg;
    memcpy(incoming_msg.mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
    memcpy(incoming_msg.data, data, len);
    incoming_msg.len = len;

    if (dataQueue) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(dataQueue, &incoming_msg, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

int RoomCommunications::findPeerIndex(uint8_t* mac_addr){
    // Find the peer index
    int peerIndex = -1;
    for (int i = 0; i < numPeers; ++i) {
        if (memcmp(peers[i].mac_addr, mac_addr, MAC_ADDRESS_LENGTH) == 0) {
            peerIndex = i;
            break;
        }
    }
    return peerIndex;
}