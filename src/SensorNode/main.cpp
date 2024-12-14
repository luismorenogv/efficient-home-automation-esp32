/**
 * @file main.cpp
 * @brief Main file for SensorNode
 * 
 * @author Luis
 * @date Dec 8, 2024
 */
#include <Arduino.h>
#include "SensorNode/SensorNode.h"
#include "config.h"

// Persisting state across deep sleep
RTC_DATA_ATTR bool first_cycle = true;
RTC_DATA_ATTR uint32_t sleep_period_ms = DEFAULT_SLEEP_DURATION;
RTC_DATA_ATTR uint8_t channel_wifi = 0;

// Create the SensorNode with references to RTC-stored variables
SensorNode sensorNode(ROOM_ID, &sleep_period_ms, &channel_wifi, &first_cycle);

void setup() {
    Serial.begin(115200);

    if (!sensorNode.initialize()) {
        LOG_ERROR("Initialization failed, going to sleep...");
        sensorNode.goSleep(false);
    }

    if (first_cycle) {
        // On first cycle, try to join the master network
        if (sensorNode.joinNetwork()) {
            first_cycle = false;
        } else {
            // Unable to connect with the master
            // Going deep sleep permanently
            sensorNode.goSleep(true);
        }
    }

    sensorNode.run();
    sensorNode.goSleep(false);
}

void loop() {
    // Not used; device sleeps after setup
}