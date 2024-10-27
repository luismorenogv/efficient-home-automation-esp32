/**
 * @file slave.h 
 * @brief Constants and Declarations of Functions for slave.ino
 *
 * @author Luis Moreno
 * @date October 26, 2024
 *
 */

#ifndef SLAVE_H
#define SLAVE_H

#include <stdint.h>
#include <esp_now.h>

// ----------------------------
// Constants
// ----------------------------

// GPIO Pins
#define DHT_PIN 4                    // GPIO pin connected to DHT22
#define PIR_PIN 26                   // GPIO pin connected to PIR sensor

// Sensor Type
#define DHT_TYPE DHT22               // Type of DHT sensor

// Timing
#define SENDING_PERIOD 60            // Sending period in seconds

// Device Masks
#define AIR_CONDITIONER_MASK 0xF0    // Bitmask for Air Conditioner
#define LIGHTS_MASK 0x0F             // Bitmask for Lights

// Master Device MAC Address
const uint8_t MASTER_ADDRESS[6] = {0x3C, 0x84, 0x27, 0xE1, 0xB2, 0xCC};

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
 * @brief Initializes ESP-NOW and adds the master device as a peer.
 */
void initializeEspNow();

/**
 * @brief Reads data from the DHT22 sensor and sends it via ESP-NOW.
 */
void readAndSendSensorData();

/**
 * @brief Halts execution
 */
void haltExecution();

#endif /* SLAVE_H */

