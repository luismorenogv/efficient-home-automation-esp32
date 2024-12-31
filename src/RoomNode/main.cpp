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
    room.run();
}

void loop() {
    // Not used; FreeRTOS tasks handle functionality
}

