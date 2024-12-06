/**
 * @file MasterCommunications.cpp
 * @brief Implementation of MasterCommunications class for MasterDevice communication
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#include "MasterDevice/MasterCommunications.h"

MasterCommunications::MasterCommunications() : CommunicationsBase() {
    instance = this;
}

void MasterCommunications::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
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
