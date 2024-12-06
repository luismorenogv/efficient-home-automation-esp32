/**
 * @file PowerManager.h
 * @brief Handles deep sleep functionality for SensorNode
 * 
 * Allows updating the sleep period based on instructions from the MasterDevice.
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */
#pragma once
#include <Arduino.h>

class PowerManager {
public:
    PowerManager(uint32_t* sleep_duration_ms);
    void enterDeepSleep();
    void enterPermanentDeepSleep();
    void updateSleepPeriod(uint32_t new_sleep_duration_ms);
    uint32_t getSleepPeriod();

private:
    uint32_t* sleep_duration_ms; // Stored in RTC memory to persist across deep sleep cycles
};
