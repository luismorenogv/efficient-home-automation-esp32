/**
 * @file slave.h 
 * @brief Constants and Declarations of Functions for slave.ino
 *
 * @author Luis Moreno
 * @date November 1, 2024
 */

#ifndef SLAVE_H
#define SLAVE_H

#include <stdint.h>
#include <esp_now.h>
#include "DHT.h"

// ----------------------------
// Constants
// ----------------------------

// GPIO Pins
constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t LDR_PIN = 35; // Pin for LDR resistor

// Sensor Type
constexpr auto DHT_TYPE = DHT22;

// Conversion Factors
constexpr uint32_t MS_TO_US = 1000;
constexpr uint32_t S_TO_MS = 1000;
constexpr uint32_t S_TO_US = 1000000;
constexpr uint32_t MIN_TO_MS = 60000;

// Communication Timing
constexpr uint32_t WAKE_UP_PERIOD_MIN = 2;        // Wake-up period in minutes
constexpr uint32_t COMM_WINDOW_DURATION_S = 6;   // Communication window duration in seconds

constexpr uint8_t SEND_DATA_INTERVAL = 3;             // Send data every 1 sleep cycle

constexpr uint8_t SYNC_INTERVAL = 5;                  // Send data every 1 sleep cycle (5 minutes)

constexpr uint32_t LEEWAY_WAKE_UP_TIME_MS = 500;       // Slave wakes up earlier than what the master expects

constexpr uint32_t IM_HERE_INTERVAL_MS = 10000;       // Send notification for joining the network to master

constexpr uint32_t DATA_TIMEOUT_MS = 3000;

constexpr uint8_t NUM_FRAMES = 5;                      

const uint8_t SIZE_OF[NUM_FRAMES] = {1, 1, 9, 2, 1}; // IM_HERE, START, SENSOR_DATA, ACK
const char* NAME[NUM_FRAMES] = {"IM_HERE", "START", "SENSOR_DATA", "ACK", "SYNC"}; // Debugging

// Master Device MAC Address
const uint8_t MASTER_ADDRESS[6] = {0x3C, 0x84, 0x27, 0xE1, 0xB2, 0xCC};

// ----------------------------
// Structures
// ----------------------------

// Message types enumeration (1 byte each)
typedef enum : uint8_t {
  IM_HERE     = 0x00, // Slave to Master: Indicates slave is present
  START       = 0x01, // Master to Slave: Instructs slave to start operations
  SENSOR_DATA = 0x02, // Slave to Master: Sends sensor data
  ACK         = 0x03, // Acknowledge message reception
  SYNC        = 0x04  // Slave to Master: Synchronize the communication window timing with the master
} MessageType;

// Generic message structure
typedef struct {
  MessageType type;   // Message type (1 byte)
  uint8_t payload[8]; // Additional data
} Message;

// Sensor data structure
typedef struct {
  float temperature;
  float humidity;
} SensorData;

// ----------------------------
// Function Declarations
// ----------------------------

/**
 * @brief Callback function called when data is sent via ESP-NOW.
 *
 * @param mac_addr MAC address of the receiver.
 * @param status   Status of the send operation.
 */
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

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

#endif /* SLAVE_H */
