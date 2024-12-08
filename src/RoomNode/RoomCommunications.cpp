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

bool RoomCommunications::waitForAck(MessageType expected_ack, unsigned long timeout_ms) {
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

void RoomCommunications::ackReceived(uint8_t* mac_addr, MessageType acked_msg){
    if (memcmp(mac_addr, master_mac_addr, sizeof(mac_addr)) == 0){
        if (acked_msg == last_acked_msg){
            ack_received = true;
            xSemaphoreGive(ackSemaphore); // Signal ACK reception
            Serial.println("ACK received from master");
        } else {
            Serial.println("Received ACK for incorrect message type.");
        }
    } else {
        Serial.println("Recevied ACK from invalid MAC address");
    }
}

void RoomCommunications::sendMsg(const uint8_t* data, size_t size) {
    // Assume only one peer (master)
    if (numPeers > 0) {
        CommunicationsBase::sendMsg(peers[0].mac_addr, data, size);
    } else {
        Serial.println("No peers registered.");
    }
}