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

// ----------------------------
// Data Structure Definitions
// ----------------------------
typedef struct {
    float temperature;  ///< Temperature data
    float humidity;     ///< Humidity data
} struct_message;

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

#endif /* __MASTER_H__ */

