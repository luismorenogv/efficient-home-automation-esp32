/**
 * @file PowerManager.h
 * @brief Declaration of PowerManager class for handling deep sleep functionality
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
    uint32_t* sleep_duration_ms;
};
