/**
 * @file slave.h 
 * @brief Constants and Declarations of Functions for slave.ino
 *
 * @author Luis Moreno
 * @date October 26, 2024
 */

#ifndef SLAVE_H
#define SLAVE_H

#include <stdint.h>
#include <esp_now.h>

// ----------------------------
// Constants
// ----------------------------

// GPIO Pins
#define DHT_PIN 4
#define LD2410_POWER_PIN 33  // Pin to control LD2410 power
#define LD2410_DATA_PIN 34

// Sensor Type
#define DHT_TYPE DHT22

// Conversion
#define MS_TO_US_FACTOR 1000
#define S_TO_MS_FACTOR 1000

// Device Masks
#define AIR_CONDITIONER_MASK 0xF0    // Bitmask for Air Conditioner
#define LIGHTS_MASK 0x0F             // Bitmask for Lights

// Communication Timing
#define WAKE_UP_PERIOD_MS 1000       // Wake-up period in ms
#define COMM_WINDOW_DURATION_MS 100  // Communication window duration in ms

#define SEND_DATA_INTERVAL_S 5      // Send data every 600 seconds (10 minutes)
#define LD2410_POLLING_INTERVAL_S 5   // Poll LD2410 every 5 seconds

// Master Device MAC Address
const uint8_t MASTER_ADDRESS[6] = {0x3C, 0x84, 0x27, 0xE1, 0xB2, 0xCC};
// ----------------------------
// Structures
// ----------------------------

/**
 * @brief Structure to hold sensor data (temperature and humidity).
 */
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

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
 * @brief Turns the lights on or off based on the state.
 *
 * @param state True to turn on, False to turn off.
 */
void turnLights(bool state);

/**
 * @brief Turns the air conditioner on or off based on the state.
 *
 * @param state True to turn on, False to turn off.
 */
void turnAirConditioner(bool state);

/**
 * @brief Initializes ESP-NOW communication.
 */
void initializeEspNow();

/**
 * @brief Configures ESP-NOW for sending data to the master device.
 */
void configureEspNowSending();

/**
 * @brief Reads data from the DHT22 sensor and sends it via ESP-NOW.
 */
void readAndSendSensorData();

/**
 * @brief Halts execution
 */
void haltExecution();

#endif /* SLAVE_H */

