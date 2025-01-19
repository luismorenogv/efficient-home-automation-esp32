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

#define sensorSerial Serial2
#define RX_PIN 16
#define TX_PIN 17
#ifdef ENABLE_LOGGING
    #define DEBUG_MODE
#endif
#include "MyLD2410.h"

MyLD2410 sensor(sensorSerial);

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

    // Returns current presence state
    bool getPresence();

    // Configures sensor parameters via UART
    bool initialize();

private:
    const uint8_t MAX_INIT_RETRIES = 3;
    const uint32_t PRESENCE_TIMEOUT = 1000;

    static void IRAM_ATTR staticPresenceISR();
    void IRAM_ATTR presenceISR();
    static LD2410* instance;
    QueueHandle_t presenceQueue;
};