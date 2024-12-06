/**
 * @file main.cpp
 * @brief Main file for SensorNode
 * 
 * The node initializes, optionally joins the network on the first run, sends data, and then goes to sleep.
 * Sleep duration and Wi-Fi channel are stored in RTC memory for persistence.
 * 
 * After each wake-up, run() is called to read and transmit data again.
 * 
 * @author Luis
 * @date Nov 25, 2024
 */
#include <Arduino.h>
#include "SensorNode/SensorNode.h"
#include "config.h"

// RTC_DATA_ATTR variables persist through deep sleep cycles
RTC_DATA_ATTR bool first_cycle = true;
RTC_DATA_ATTR uint32_t sleep_period_ms = DEFAULT_SLEEP_DURATION;
RTC_DATA_ATTR uint8_t channel_wifi = 0;

SensorNode sensorNode(ROOM_ID, &sleep_period_ms, &channel_wifi, &first_cycle);

void setup() {
    Serial.begin(115200);

    if (!sensorNode.initialize()) {
        Serial.println("Initialization failed, sleeping...");
        sensorNode.goSleep();
    }

    if (first_cycle) {
        // On first cycle, try to join the master network
        if (sensorNode.joinNetwork()) {
            first_cycle = false;
        } else {
            sensorNode.goSleep();
        }
    }

    sensorNode.run();
    sensorNode.goSleep();
}

void loop() {
    // Unused; device sleeps after setup completes
}
