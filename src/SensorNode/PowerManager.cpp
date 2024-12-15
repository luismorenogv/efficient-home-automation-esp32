/**
 * @file PowerManager.cpp
 * @brief Implementation of PowerManager class for handling deep sleep functionality
 * 
 * @author Luis
 * @date Dec 8, 2024
 */
#include "SensorNode/PowerManager.h"

PowerManager::PowerManager(uint32_t* sleep_duration_ms) : sleep_duration_ms(sleep_duration_ms) {}

void PowerManager::enterDeepSleep() {
    LOG_INFO("Entering deep sleep");
    // Convert ms to µs for esp_sleep_enable_timer_wakeup
    esp_sleep_enable_timer_wakeup(*sleep_duration_ms * 1000ULL);
    esp_deep_sleep_start();
}

void PowerManager::enterPermanentDeepSleep() {
    LOG_INFO("Entering permanent deep sleep");
    esp_deep_sleep_start();
}

void PowerManager::updateSleepPeriod(uint32_t new_sleep_duration_ms) {
    *sleep_duration_ms = new_sleep_duration_ms;
    LOG_INFO("Updated sleep period: %u ms", *sleep_duration_ms);
}

uint32_t PowerManager::getSleepPeriod() {
    return *sleep_duration_ms;
}