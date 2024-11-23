/**
 * @file PowerManager.h
 * @brief Declaration of PowerManager class for handling deep sleep functionality
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#pragma once
#include <Arduino.h>

class PowerManager {
public:
    PowerManager(unsigned long wake_interval_ms);
    void enterDeepSleep();
    void enterPermanentDeepSleep();
    void updateSleepPeriod(uint32_t new_wake_interval_ms);
    uint32_t returnWakeInterval();

private:
    unsigned long wake_interval_ms;
};
