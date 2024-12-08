/**
 * @file LD2410.h
 * @brief Declaration of LD2410 class for managing presence sensor
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
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

    void setQueue(QueueHandle_t queue);
    void start();

    bool initialize(); // New method to configure LD2410

private:
    static void IRAM_ATTR staticPresenceISR();
    void IRAM_ATTR presenceISR();

    static LD2410* instance;
    QueueHandle_t presenceQueue;

    // Utility methods for configuration:
    bool sendCommand(const uint8_t* cmd, size_t len);
    bool waitForAck(uint16_t expectedCmdWord, uint8_t expectedStatus = 0);

    // Reading from UART:
    bool readResponse(uint8_t* buffer, size_t bufferSize, uint32_t timeoutMs = 100);
};

