/**
 * @file main.cpp
 * @brief Main file for RoomNode
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include <Arduino.h>
#include "RoomNode/RoomNode.h"

RoomNode room(ROOM_ID);

void setup() {
    // Initializes room and tries to join the master network, then runs node logic
    room.initialize();
    if(!room.joinNetwork()){
        Serial.println("No connection to Master. Sleeping 30min...");
        esp_sleep_enable_timer_wakeup(30*60*1000000);
        esp_deep_sleep_start();
    }
    room.run();
}

void loop() {
    // Not used; FreeRTOS tasks handle functionality
}

