/**
 * @file LD2410.cpp
 * @brief Implementation of LD2410 class of RoomNode
 *
 * @author 
 * @date Dec 4, 2024
 */

#include "RoomNode/LD2410.h"

// Initialize the static instance pointer to nullptr
LD2410* LD2410::instance = nullptr;

LD2410::LD2410(QueueHandle_t queue) : presenceQueue(queue) {
    instance = this; // Assign the current instance to the static pointer

    pinMode(LD2410_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(LD2410_PIN), staticPresenceISR, CHANGE);
}

LD2410::~LD2410() {
    // Clean up: Detach the interrupt and reset the instance pointer
    detachInterrupt(digitalPinToInterrupt(LD2410_PIN));
    instance = nullptr;
}

void IRAM_ATTR LD2410::staticPresenceISR() {
    if (instance) {
        instance->presenceISR();
    }
}

void IRAM_ATTR LD2410::presenceISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t presenceState = digitalRead(LD2410_PIN);
    xQueueSendFromISR(presenceQueue, &presenceState, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
