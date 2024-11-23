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

RTC_DATA_ATTR bool first_cycle = true;

SensorNode sensorNode(ROOM_ID);

void setup() {
    sensorNode.initialize();
    if (first_cycle){
        if (sensorNode.joinNetwork()){
            first_cycle = false;
        } else{
            sensorNode.goSleep();
        }
    }
    sensorNode.run();
    sensorNode.goSleep();
}

void loop() {
    // Not used as the device sleeps after setup
}

