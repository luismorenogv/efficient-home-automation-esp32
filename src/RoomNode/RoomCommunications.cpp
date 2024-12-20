/**
 * @file RoomNodeCommunications.cpp
 * @brief Implementation of RoomNodeCommunications class for RoomNode communication
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "RoomNode/RoomCommunications.h"

RoomCommunications::RoomCommunications()
    : ack_received(false), last_acked_msg(MessageType::ACK), CommunicationsBase() {
    instance = this;
    ackSemaphore = xSemaphoreCreateBinary();
}

// Waits for an ACK of expected type or times out
bool RoomCommunications::waitForAck(MessageType expected_ack, unsigned long timeout_ms) {
    ack_received = false;
    last_acked_msg = expected_ack;
    return (xSemaphoreTake(ackSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) && ack_received;
}

// Called when an ACK is received, releases semaphore if it matches expected ACK
void RoomCommunications::ackReceived(uint8_t* mac_addr, MessageType acked_msg) {
    if (memcmp(mac_addr, master_mac_addr, sizeof(mac_addr)) == 0) {
        if (acked_msg == last_acked_msg) {
            ack_received = true;
            xSemaphoreGive(ackSemaphore);
            LOG_INFO("ACK received from master");
        } else {
            LOG_WARNING("ACK for incorrect message type.");
        }
    } else {
        LOG_WARNING("ACK from invalid MAC address");
    }
}

// Sends data to the master (assumes only one peer)
void RoomCommunications::sendMsg(const uint8_t* data, size_t size) {
    if (numPeers > 0) {
        CommunicationsBase::sendMsg(peers[0].mac_addr, data, size);
    } else {
        LOG_WARNING("No peers registered.");
    }
}

// Handles received data by enqueuing it for processing
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
