/**
 * @file NTPClient.cpp
 * @brief Implementation of NTPClient class for NTP time synchronization
 * 
 * Manages synchronization of system time using NTP servers.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "Common/NTPClient.h"

NTPClient::NTPClient() {
    // Constructor can initialize variables if needed
}

// Initializes the NTP client and synchronizes time
void NTPClient::initialize() {
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;

    // Wait until time is synchronized
    int retry = 0;
    const int retry_count = 10;
    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        LOG_INFO("Waiting for NTP time synchronization");
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }

    if (retry == retry_count) {
        LOG_ERROR("Failed to synchronize time after multiple attempts");
    } else {
        time_t now = time(nullptr);
        LOG_INFO("Time synchronized: %ld", now);
        #ifdef ENABLE_LOGGING
            Serial.println(&timeinfo, "[INFO] Current time: %A, %B %d %Y %H:%M:%S");
        #endif
    }
}

// Checks if the system time is valid
bool NTPClient::isTimeValid() {
    time_t now = time(nullptr);
    return now > 1732288146; // Set to Nov 22, 2024
}
