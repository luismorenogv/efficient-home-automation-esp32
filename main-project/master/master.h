/**
 * @file master.h
 * @brief Constants and Function Declarations for master.ino
 *
 * @author Luis Moreno
 * @date October 27, 2024
 */

#ifndef __MASTER_H__
#define __MASTER_H__

#include <stdint.h>
#include <esp_now.h>

// ----------------------------
// Constants
// ----------------------------

#define DATA_ARRAY_SIZE 144  // Size of the data arrays for temperature and humidity
#define MAX_SLAVES 3         // Maximum number of slaves

#define NUM_FRAMES 6

const uint8_t FRAME_SIZE[NUM_FRAMES] = { 1, 1, 9, 2, 2, 5};

// Slave Device MAC Address
const uint8_t SLAVE_ADDRESS[6] = {0xEC, 0x64, 0xC9, 0x5E, 0x83, 0x54}; // Dev Board
// ----------------------------
// Data Structure Definitions
// ----------------------------

// Message types enumeration
typedef enum : uint8_t {
    IM_HERE     = 0x01, // Slave to Master: Indicates slave is present
    START       = 0x02, // Master to Slave: Instructs slave to start operations
    SENSOR_DATA = 0x03, // Slave to Master: Sends sensor data
    LIGHTS      = 0x04, // Master to Slave: Control lights
    AC          = 0x05, // Master to Slave: Control air conditioner
    SYNC        = 0x06  // Master to Slave: Synchronize time
} MessageType;

// Sensor data structure
typedef struct {
    float temperature;  ///< Temperature data
    float humidity;     ///< Humidity data
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
void setup();

/**
 * @brief No utility.
 */
void loop();

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
 * @brief Initializes ESP-NOW communication and configures sending to the slave device.
 */
void initializeEspNow();

#endif /* __MASTER_H__ */

