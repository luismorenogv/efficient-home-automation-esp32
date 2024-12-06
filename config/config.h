/**
 * @file config.h
 * @brief Configuration constants for MasterDevice and SensorNode
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */
#pragma once
#include <Arduino.h>
#include "Common/mac_addrs.h"

/**************************************************************
 *                         Common                             *
 *************************************************************/
constexpr unsigned long DEFAULT_SLEEP_DURATION = 300000; // 5min/15min/30min/1h/3h/6h

constexpr const uint8_t* master_mac_addr = esp32s3_mac;

/**************************************************************
 *                      Master Device                         *
 *************************************************************/
#ifdef MODE_MASTER
// Number of rooms monitored by the MasterDevice
constexpr uint8_t NUM_ROOMS = 3;

// Maximum number of data points to store per room
constexpr uint16_t MAX_DATA_POINTS = 300;

// Names of the rooms
constexpr const char* ROOM_NAME[NUM_ROOMS] = {"Room Luis", "Room Pablo", "Room Ana"};

constexpr uint8_t MAX_PEERS = NUM_ROOMS;
#endif

/**************************************************************
 *                      Sensor node                           *
 *************************************************************/
#ifdef MODE_SENSOR
// Define Room ID (unique for each SensorNode)
constexpr uint8_t ROOM_ID = 1;

// Communication
constexpr unsigned long ACK_TIMEOUT_MS = 5000;     // 5 seconds
constexpr uint8_t MAX_RETRIES = 2;
constexpr uint8_t MAX_INIT_RETRIES = 3;

// DHT Sensor configuration
constexpr uint8_t DHT_PIN = 4; // GPIO pin connected to DHT22

constexpr uint8_t MAX_PEERS = 1;
#endif
/**************************************************************
 *                      Room node                             *
 *************************************************************/
#ifdef MODE_ROOM
constexpr uint8_t LD2410_PIN = 14;
constexpr uint8_t MAX_PEERS = 1;
#endif