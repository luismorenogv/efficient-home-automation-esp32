/**
 * @file LD2410.h
 * @brief Manages LD2410 presence sensor configuration and interrupts.
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <Arduino.h>
#include <freertos/queue.h>
#include "config.h"

class LD2410 {
public:
    LD2410();
    ~LD2410();

    LD2410(const LD2410&) = delete;
    LD2410& operator=(const LD2410&) = delete;

    // Sets the queue to post presence states
    void setQueue(QueueHandle_t queue);

    // Starts the interrupt for presence detection
    void start();

    // Configures sensor parameters via UART
    bool initialize();

private:
    static void IRAM_ATTR staticPresenceISR();
    void IRAM_ATTR presenceISR();

    // Sends a config command to the sensor
    bool sendCommand(const uint8_t* cmd, size_t len);

    // Waits for an ACK response to the last command
    bool waitForAck(uint16_t expectedCmdWord, uint8_t expectedStatus = 0);

    // For potential future use: reads UART response
    bool readResponse(uint8_t* buffer, size_t bufferSize, uint32_t timeoutMs = 100);

    static LD2410* instance;
    QueueHandle_t presenceQueue;
};