/**
 * @file slave.h 
 * @brief Constants and Declarations of Functions for slave.ino
 *
 * @author Luis Moreno
 * @date November 1, 2024
 */

#ifndef SLAVE_H
#define SLAVE_H

#include <esp_now.h>
#include "DHT.h"
#include "common.h"

// ----------------------------
// Constants
// ----------------------------

// GPIO Pins
constexpr uint8_t DHT_PIN = 4;

// Sensor Type
constexpr auto DHT_TYPE = DHT22;

// Conversion Factors
constexpr uint32_t MS_TO_US = 1000;
constexpr uint32_t S_TO_MS = 1000;
constexpr uint32_t MIN_TO_MS = 60000;

// Communication Timing
constexpr uint32_t DEFAULT_WAKE_UP_MS = (2 * MIN_TO_MS);
constexpr uint32_t DEFAULT_TIME_AWAKE_MS = 5000;

constexpr uint8_t SENSOR_DATA_PERIOD = 4;
constexpr uint8_t SYNC_PERIOD = 6;

constexpr uint16_t ACK_TIMEOUT_MS = 1000;
constexpr uint16_t IM_HERE_TIMEOUT_MS = 2000;

// Master Device MAC Address
const uint8_t MASTER_ADDRESS[MAC_ADDRESS_LENGTH] = {0x3C, 0x84, 0x27, 0xE1, 0xB2, 0xCC};

// ----------------------------
// Function Declarations
// ----------------------------

/**
 * @brief Callback function called when data is received via ESP-NOW.
 *
 * @param info Information about the received message.
 * @param incoming_data Pointer to the received data.
 * @param len Length of the received data.
 */
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len);

/**
 * @brief Initializes ESP-NOW communication.
 */
void initializeEspNow();

/**
 * @brief Reads and returns data from the DHT22 sensor.
 */
SensorData readSensorData();

/**
 * @brief Halts execution.
 */
void haltExecution();

/**
 * @brief Sends a message to a destination device.
 *
 * @param dest_mac MAC address of the destination.
 * @param msg_type Type of the message.
 * @param payload Optional payload data.
 */
void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload = nullptr);

/**
 * @brief Sends the IM_HERE message with wake-up configurations.
 */
void sendImHereMessage();

/**
 * @brief Sends sensor data to the master.
 */
void sendSensorData();

/**
 * @brief Sends a SYNC message to the master.
 */
void sendSyncMessage();

#endif /* SLAVE_H */
