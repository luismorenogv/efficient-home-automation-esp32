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

    // Delete copy constructor and assignment operator to prevent multiple instances
    LD2410(const LD2410&) = delete;
    LD2410& operator=(const LD2410&) = delete;

    void setQueue(QueueHandle_t queue);

    void start();

private:
    static void IRAM_ATTR staticPresenceISR();
    void IRAM_ATTR presenceISR();

    static LD2410* instance; // Static pointer to the class instance
    QueueHandle_t presenceQueue;
};
