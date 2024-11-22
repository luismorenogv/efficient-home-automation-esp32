/**
 * @file MasterConfig.h
 * @brief Configuration constants for MasterDevice
 * 
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#pragma once

#include <Arduino.h>
#include "mac_addrs.h"

// Number of rooms monitored by the MasterDevice
constexpr uint8_t NUM_ROOMS = 3;

// Number of available SensorNodes
constexpr uint8_t AVAILABLE_NODES = 1;

// Maximum number of data points to store per room
constexpr uint16_t MAX_DATA_POINTS = 300;

// Names of the rooms
constexpr const char* ROOM_NAME[NUM_ROOMS] = {"Room Luis", "Room Pablo", "Room Ana"};

// Define an array of pointers to MAC addresses
constexpr const uint8_t* NODE_MAC_ADDRS[AVAILABLE_NODES] = {
    esp32dev_mac
};