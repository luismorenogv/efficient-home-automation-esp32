/**
 * @file main.cpp
 * @brief Main file for SensorNode
 * 
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include <Arduino.h>
#include "SensorNode/SensorNode.h"
#include "SensorConfig.h"


SensorNode sensorNode(ROOM_ID);

void setup() {
    sensorNode.initialize();
    sensorNode.run();
    sensorNode.goSleep();
}

void loop() {
    // Not used as the device sleeps after setup
}

