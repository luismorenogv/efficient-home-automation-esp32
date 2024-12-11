/**
 * @file PowerManager.h
 * @brief Handles deep sleep functionality for SensorNode
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */
#pragma once
#include <Arduino.h>

class PowerManager {
public:
    // Constructs with pointer to sleep duration stored in RTC memory
    PowerManager(uint32_t* sleep_duration_ms);

    // Enters deep sleep for the configured duration
    void enterDeepSleep();

    // Enters deep sleep indefinitely
    void enterPermanentDeepSleep();

    // Updates the sleep duration
    void updateSleepPeriod(uint32_t new_sleep_duration_ms);

    // Retrieves the current sleep period
    uint32_t getSleepPeriod();

private:
    uint32_t* sleep_duration_ms; // Stored across deep sleep cycles
};