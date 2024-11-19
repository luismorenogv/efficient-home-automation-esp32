/**
 * @file main.cpp 
 * @brief Main file for Sensor Node.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#include "SensorNode.h"

// Define Room ID (unique for each sensor node)
constexpr uint8_t ROOM_ID = 0;

SensorNode sensorNode(ROOM_ID);

void setup() {
    sensorNode.initialize();

    if (sensorNode.waitForAck()) {
        Serial.println("Data successfully sent and ACK received");
    } else {
        Serial.println("Data transmission failed");
    }
    
    // Enter deep sleep for the wake_interval duration
    sensorNode.enterDeepSleep();
}

void loop() {
    // Not needed as the device sleeps after setup
}

