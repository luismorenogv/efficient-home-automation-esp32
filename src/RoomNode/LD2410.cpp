/**
 * @file LD2410.cpp
 * @brief Implementation of LD2410 class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "RoomNode/LD2410.h"

LD2410* LD2410::instance = nullptr;

#define LD2410_BAUDRATE 256000
#define LD2410_SERIAL Serial2

LD2410::LD2410(): sensor(sensorSerial) {
    instance = this;
    pinMode(LD2410_PIN, INPUT);
}

LD2410::~LD2410() {
    detachInterrupt(digitalPinToInterrupt(LD2410_PIN));
    instance = nullptr;
}

// Sets the queue for presence states
void LD2410::setQueue(QueueHandle_t queue) {
    presenceQueue = queue;
}

// Starts the interrupt-based presence detection
void LD2410::start() {
    uint8_t presenceState = digitalRead(LD2410_PIN);
    LOG_INFO("Presence sensor initial state: %s", presenceState == HIGH ? "PRESENCE" : "NO PRESENCE" );
    xQueueSend(presenceQueue, &presenceState, 0);
    attachInterrupt(digitalPinToInterrupt(LD2410_PIN), staticPresenceISR, CHANGE);
}

void IRAM_ATTR LD2410::staticPresenceISR() {
    if (instance) {
        instance->presenceISR();
    }
}

// ISR to send presence state to a queue for processing
void IRAM_ATTR LD2410::presenceISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t presenceState = digitalRead(LD2410_PIN);
    xQueueSendFromISR(presenceQueue, &presenceState, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

bool LD2410::getPresence(){
    if (digitalRead(LD2410_PIN) == HIGH){
        return true;
    } else {
        return false;
    }
}