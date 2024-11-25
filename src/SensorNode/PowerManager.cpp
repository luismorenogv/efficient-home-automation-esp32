/**
 * @file PowerManager.cpp
 * @brief Implementation of PowerManager class for handling deep sleep functionality
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "SensorNode/PowerManager.h"

PowerManager::PowerManager(uint32_t* sleep_duration_ms) : sleep_duration_ms(sleep_duration_ms) {
}

void PowerManager::enterDeepSleep() {
    Serial.println("Entering deep sleep");
    esp_sleep_enable_timer_wakeup(*sleep_duration_ms * 1000); // Convert ms to us
    esp_deep_sleep_start();
}

void PowerManager::enterPermanentDeepSleep() {
    Serial.println("Disconnecting from the network");
    esp_deep_sleep_start();
}

void PowerManager::updateSleepPeriod(uint32_t new_sleep_duration_ms){
    *sleep_duration_ms = new_sleep_duration_ms;
    Serial.printf("New sleep period set to %u ms\r\n", *sleep_duration_ms);
}

uint32_t PowerManager::getSleepPeriod(){
    return *sleep_duration_ms;
}
