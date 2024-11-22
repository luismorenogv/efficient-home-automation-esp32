/**
 * @file PowerManager.cpp
 * @brief Implementation of PowerManager class for handling deep sleep functionality
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include "SensorNode/PowerManager.h"
#include <esp_sleep.h>
#include <Arduino.h>

PowerManager::PowerManager(unsigned long wake_interval_ms) : wake_interval_ms(wake_interval_ms) {
}

void PowerManager::enterDeepSleep() {
    Serial.println("Entering deep sleep");
    esp_sleep_enable_timer_wakeup(wake_interval_ms * 1000); // Convert ms to us
    esp_deep_sleep_start();
}

void PowerManager::enterPermanentDeepSleep() {
    Serial.println("Disconnecting from the network");
    esp_deep_sleep_start();
}