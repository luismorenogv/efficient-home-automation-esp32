/**
 * @file master.h
 * @brief Constants and Function Declarations for master.ino
 *
 * @author Luis Moreno
 * @date November 1, 2024
 */

#ifndef __MASTER_H__
#define __MASTER_H__

#include <stdint.h>
#include <esp_now.h>

// ----------------------------
// Constants
// ----------------------------

constexpr uint16_t DATA_ARRAY_SIZE = 144;  // Size of the data arrays for temperature and humidity
constexpr uint8_t MAX_SLAVES = 3;          // Maximum number of slaves

constexpr uint32_t MIN_TO_MS_FACTOR = 60000;

// Communication Timing
constexpr uint32_t WAKE_UP_PERIOD_MS = (2 * MIN_TO_MS_FACTOR); // Wake-up period in ms (1 minute)
constexpr uint32_t COMM_WINDOW_DURATION_MS = 6000;             // Communication window duration in ms

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

// Sensor data structure
typedef struct {
  float temperature;  // Temperature data
  float humidity;     // Humidity data
} SensorData;

// General message structure
typedef struct {
  MessageType type;   // Message type (1 byte)
  uint8_t payload[8]; // Additional data
} Message;

// Slave information structure
typedef struct {
  uint8_t mac_addr[6];       // MAC address of the slave
  uint32_t last_sync_time;   // Last synchronization time (millis)
} SlaveInfo;

// ----------------------------
// Function Declarations
// ----------------------------

/**
 * @brief Callback function called when data is received via ESP-NOW.
 *
 * @param info Information about the received packet.
 * @param incoming_data Pointer to the received data.
 * @param len Length of the received data.
 */
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len);

/**
 * @brief Initializes ESP-NOW communication and sets up device configuration.
 */
void initializeEspNow();

/**
 * @brief Halts execution on failure.
 */
void haltExecution();

/**
 * @brief Handles a new slave joining the network.
 *
 * @param slave_mac MAC address of the new slave device.
 */
void handleNewSlave(const uint8_t *slave_mac);

/**
 * @brief Sends a message to a slave device.
 *
 * @param dest_mac MAC address of the slave.
 * @param msg_type Type of the message.
 * @param payload Optional payload data.
 */
void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload = nullptr);

#endif /* __MASTER_H__ */
