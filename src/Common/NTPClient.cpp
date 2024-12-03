/**
 * @file NTPClient.cpp
 * @brief Implementation of NTPClient class for NTP time synchronization
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "Common/NTPClient.h"
#include <time.h>
#include <Arduino.h>

NTPClient::NTPClient() {
}

void NTPClient::initialize() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;

    // Wait until time is synchronized
    int retry = 0;
    const int retry_count = 10;
    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        Serial.println("Waiting for NTP time synchronization");
        delay(2000);
        retry++;
    }

    if (retry == retry_count) {
        Serial.println("Failed to synchronize time after multiple attempts");
    } else {
        time_t now = time(nullptr);
        Serial.printf("Time synchronized: %ld\r\n", now);
        Serial.println(&timeinfo, "Current time: %A, %B %d %Y %H:%M:%S");
    }
}

bool NTPClient::isTimeValid() {
    time_t now = time(nullptr);
    return now > 1732288146; // Set to Nov 22, 2024
}
