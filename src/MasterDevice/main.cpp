/**
 * @file MasterDevice.cpp 
 * @brief Main code of the master device
 * 
 * @author Luis Moreno
 * @date Nov 19, 2024
 */

#include "MasterDevice.h"

void setup() {
    MasterDevice::getInstance()->start();
}

void loop() {
    // Empty loop since tasks handle functionality
    vTaskDelete(NULL); // Delete the loop task
}


