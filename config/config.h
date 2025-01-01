/**
 * @file config.h
 * @brief Configuration constants for MasterDevice, RoomNodes and sensorNodes
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */
#pragma once

#define ENABLE_LOGGING 1  // Set to 0 to disable all logs

#include "Logger.h"
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
constexpr uint8_t NUM_ROOMS = 6;                             // Number of rooms managed
constexpr uint16_t MAX_DATA_POINTS = 300;                    // Maximum sensor data points per room
constexpr const char* ROOM_NAME[NUM_ROOMS] = {"Dormitorio Luis", "Dormitorio Pablo", "Dormitorio Ana",
                                              "Cocina", "Salón", "Coladuría"}; // Names of the rooms
constexpr uint8_t MAX_PEERS = 10;                            // Maximum number of peers

constexpr const uint32_t WEB_SERVER_PERIOD = 300;             // Web server update period in ms
constexpr const uint32_t NTPSYNC_PERIOD = 5 * 60 * 1000;      // NTP synchronization period
constexpr const uint32_t CHECK_PENDING_MSG_PERIOD = 1000;     // Period to check pending messages in ms

constexpr const uint32_t HEARTBEAT_TIMEOUT = 4 * 60 * 1000;
#endif


/**************************************************************
 *                      Sensor Node                           *
 *************************************************************/

#ifdef MODE_SENSOR
constexpr uint8_t ROOM_ID = 3;                       // Identifier for the room (Unique for each SensorNode)

constexpr unsigned long ACK_TIMEOUT_MS = 1000;       // Acknowledgment timeout in ms
constexpr uint8_t MAX_RETRIES = 2;                   // Maximum number of retries for sending messages
constexpr uint8_t MAX_INIT_RETRIES = 3;              // Maximum initialization retries
constexpr uint8_t MAX_PEERS = 1;                     // Maximum number of peers
#endif

/**************************************************************
 *                      Room Node                             *
 *************************************************************/
#ifdef MODE_ROOM
constexpr uint8_t ROOM_ID = 0;                       // Identifier for the room (Unique for each RoomNode)

// Define I2C pins for TSL2591
constexpr uint8_t I2C_SDA_PIN = 21;                   // GPIO21 for SDA
constexpr uint8_t I2C_SCL_PIN = 22;                   // GPIO22 for SCL

constexpr uint8_t LD2410_PIN = 14;                   // GPIO pin for LD2410 sensor
constexpr uint8_t MAXIMUM_MOVING_DISTANCE_GATE = 8;  // Maximum moving distance for gate activation
constexpr uint8_t MAXIMUM_STILL_DISTANCE_GATE = 8;   // Maximum still distance for gate activation
constexpr uint8_t UNMANNED_DURATION_S = 60;          // Duration in seconds before marking as unmanned
constexpr uint8_t SENSITIVITY = 100;                 // Sensitivity level for motion detection

constexpr uint8_t MAX_PEERS = 1;                     // Maximum number of peers
constexpr unsigned long ACK_TIMEOUT_MS = 1000;       // Acknowledgment timeout in ms
constexpr uint8_t MAX_RETRIES = 2;                   // Maximum number of retries for sending messages

constexpr uint8_t DEFAULT_HOUR_COLD = 9;             // Default hour for cold mode activation
constexpr uint8_t DEFAULT_MIN_COLD = 30;             // Default minute for cold mode activation
constexpr uint8_t DEFAULT_HOUR_WARM = 19;            // Default hour for warm mode activation
constexpr uint8_t DEFAULT_MIN_WARM = 0;              // Default minute for warm mode activation

constexpr uint32_t LIGHTS_CONTROL_PERIOD = 1000;      // Lights control update period in ms
constexpr uint32_t NTPSYNC_PERIOD = 5 * 60 * 1000;    // NTP synchronization period
constexpr uint32_t HEARTBEAT_PERIOD = 2 * 60 * 1000;  // Heartbeat msg sending period

constexpr uint8_t IR_LED_PIN = 15;                    // GPIO pin for IR LED
constexpr uint8_t TRANSMITTER_PIN = 13;               // GPIO pin for transmitter
#endif