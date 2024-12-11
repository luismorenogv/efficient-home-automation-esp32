/**
 * @file main.cpp
 * @brief Main file for MasterDevice
 * 
 * Initializes and starts the MasterController, which handles all system functionalities.
 * 
 * @date Dec 8, 2024
 */

#include <Arduino.h>
#include "MasterDevice/MasterController.h"

// Instantiate the MasterController
MasterController controller;

void setup() {
    controller.initialize(); // Initialize all components
}

void loop() {
    // Not used as tasks handle functionality
}
