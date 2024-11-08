/**
 * @file slave.h 
 * @brief Constants and Declarations of Functions for slave.ino
 * 
 * @author Luis Moreno
 * @date November 8, 2024
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

// Leeway given the time the processor uses to go sleep and wake up
constexpr uint32_t LEEWAY_MS = 45;

// Communication Timing
constexpr uint32_t DEFAULT_WAKE_UP_MS = 1000;
constexpr uint32_t DEFAULT_TIME_AWAKE_MS = 200;

constexpr uint8_t SENSOR_DATA_PERIOD = 2;
constexpr uint16_t IM_HERE_TIMEOUT_MS = 2000;
constexpr uint16_t MSG_TIMEOUT_MS = 100;

constexpr uint8_t MAX_RETRIES = 2; // Maximum number of retries for sending messages

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
 *
 * @return SensorData Struct containing temperature and humidity.
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
 * @param size Size of the message to send.
 */
void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload = nullptr, size_t size = 0);

/**
 * @brief Sends the IM_HERE message with wake-up configurations.
 */
void sendImHereMessage();

/**
 * @brief Sends sensor data to the master.
 *
 * @param data_to_send Contains the id, temperature, and humidity of the slave.
 */
void sendSensorData(const SensorData& data_to_send);

/**
 * @brief Handles ACTION messages received from the master.
 *
 * @param payload Pointer to the payload containing actions.
 * @param payload_len Length of the payload.
 */
void handleAction(const uint8_t *payload, uint8_t payload_len);

#endif /* SLAVE_H */

