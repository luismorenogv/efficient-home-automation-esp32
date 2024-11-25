/**
 * @file main.cpp
 * @brief Main file for MasterDevice
 * @date Nov 25, 2024
 */

#include <Arduino.h>
#include "MasterDevice/MasterController.h"

MasterController controller;

void setup() {
    controller.initialize();
}

void loop() {
    // Not used as tasks handle functionality
}


