/**
 * @file main.cpp
 * @brief Main file for SensorNode
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include <Arduino.h>
#include "SensorNode/SensorNode.h"
#include "config.h"

RTC_DATA_ATTR bool first_cycle = true;
RTC_DATA_ATTR uint32_t sleep_period_ms = DEFAULT_SLEEP_DURATION;
RTC_DATA_ATTR uint8_t channel_wifi = 0;

SensorNode sensorNode(ROOM_ID, &sleep_period_ms, &channel_wifi, &first_cycle);

void setup() {
    Serial.begin(115200);

    if (!sensorNode.initialize()) {
        Serial.println("Initialization failed. Entering deep sleep.");
        sensorNode.goSleep();
    }

    if (first_cycle) {
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
    // Not used as the device sleeps after setup
}