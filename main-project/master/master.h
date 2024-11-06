/**
 * @file master.h
 * @brief Constants and Function Declarations for master.ino
 *
 * @author Luis Moreno
 * @date November 1, 2024
 */

#ifndef __MASTER_H__
#define __MASTER_H__

#include <esp_now.h>
#include "common.h"

// ----------------------------
// Constants
// ----------------------------

constexpr uint16_t DATA_ARRAY_SIZE = 144;  // Size of the data arrays for temperature and humidity
constexpr uint8_t MAX_SLAVES = 3;          // Maximum number of slaves

constexpr uint32_t MIN_TO_MS = 60000;

const uint32_t NOT_SYNCED = 0;
// ----------------------------
// Structures
// ----------------------------

// Slave information structure
typedef struct {
  uint8_t id = NO_ID;                // ID assigned to the slave by the master
  uint8_t mac_addr[6];               // MAC address of the slave
  uint32_t wake_up_period_ms;        // Time between each wake-up in milliseconds
  uint32_t time_awake_ms;            // Time the slave stays awake in milliseconds
  uint32_t last_sync_time;           // Last synchronization time (millis)
  float temperature[DATA_ARRAY_SIZE]; // Array holding temperatures from the slave
  float humidity[DATA_ARRAY_SIZE];   // Array holding humidity values from the slave
  uint16_t data_index = 0;
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
 * @param payload Pointer to the payload containing wake_up_period_ms and time_awake_ms.
 */
void handleNewSlave(const uint8_t *slave_mac, const uint8_t *payload);

/**
 * @brief Sends a message to a slave device.
 *
 * @param dest_mac MAC address of the slave.
 * @param msg_type Type of the message.
 * @param payload Optional payload data.
 * @param size Size of the message to send.
 */
void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload = nullptr, size_t size = 0);

#endif /* __MASTER_H__ */

