/**
 * @file LD2410.cpp
 * @brief Implementation of LD2410 class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/LD2410.h"

// Initialize the static instance pointer to nullptr
LD2410* LD2410::instance = nullptr;

LD2410::LD2410(){
    instance = this; // Assign the current instance to the static pointer

    pinMode(LD2410_PIN, INPUT);
}

LD2410::~LD2410() {
    // Clean up: Detach the interrupt and reset the instance pointer
    detachInterrupt(digitalPinToInterrupt(LD2410_PIN));
    instance = nullptr;
}

void LD2410::setQueue(QueueHandle_t queue){
    presenceQueue = queue;
}

void LD2410::start(){
    attachInterrupt(digitalPinToInterrupt(LD2410_PIN), staticPresenceISR, CHANGE);
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
