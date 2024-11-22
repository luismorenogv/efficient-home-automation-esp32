/**
 * @file PowerManager.h
 * @brief Declaration of PowerManager class for handling deep sleep functionality
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#pragma once

class PowerManager {
public:
    PowerManager(unsigned long wake_interval_ms);
    void enterDeepSleep();
    void enterPermanentDeepSleep();

private:
    unsigned long wake_interval_ms;
};
