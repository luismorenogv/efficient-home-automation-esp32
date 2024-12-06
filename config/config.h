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
constexpr unsigned long DEFAULT_SLEEP_DURATION = 300000; // 5 minutes by default
constexpr const uint8_t* master_mac_addr = esp32s3_mac;

/**************************************************************
 *                      Master Device                         *
 *************************************************************/
#ifdef MODE_MASTER
constexpr uint8_t NUM_ROOMS = 3;
constexpr uint16_t MAX_DATA_POINTS = 300;
constexpr const char* ROOM_NAME[NUM_ROOMS] = {"Room Luis", "Room Pablo", "Room Ana"};
constexpr uint8_t MAX_PEERS = NUM_ROOMS;
#endif

/**************************************************************
 *                      Sensor node                           *
 *************************************************************/
#ifdef MODE_SENSOR
constexpr uint8_t ROOM_ID = 1;
constexpr unsigned long ACK_TIMEOUT_MS = 5000;     // 5 seconds
constexpr uint8_t MAX_RETRIES = 2;
constexpr uint8_t MAX_INIT_RETRIES = 3;
constexpr uint8_t DHT_PIN = 4; // GPIO pin for old DHT22 sensor (unused now, kept for reference)
constexpr uint8_t MAX_PEERS = 1;
#endif

/**************************************************************
 *                      Room node                             *
 *************************************************************/
#ifdef MODE_ROOM
constexpr uint8_t ROOM_ID = 0;
constexpr uint8_t LD2410_PIN = 14;
constexpr uint8_t MAX_PEERS = 1;
#endif
