/**
 * @file main.cpp 
 * @brief Main file for Master Node.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#include "MasterDevice.h"

MasterDevice master;

void setup() {
    master.initialize();
}

void loop() {
    // Nothing needed here as operations are handled asynchronously
}

