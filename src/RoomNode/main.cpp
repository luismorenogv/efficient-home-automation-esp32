/**
 * @file main.cpp
 * @brief Main file for RoomNode
 * 
 * @author Luis Moreno
 * @date Dec 2, 2024
 */

#include <Arduino.h>
#include "RoomNode/RoomNode.h"

RoomNode room(ROOM_ID);

void setup() {
    room.initialize();
    if(!room.joinNetwork()){
        Serial.println("Couldn't establish connection with Master. Proceeding to sleep for 30min...");
        esp_sleep_enable_timer_wakeup(30*60*1000000);
        esp_deep_sleep_start();
    }
    room.run();
}

void loop() {
    // Not used as tasks handle functionality
}


