/**
 * @file common.h
 * @brief Common definitions and structures for master and slave devices.
 * @author Luis Moreno
 * @date November 1, 2024
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// Constants
constexpr uint8_t TOTAL_FRAMES = 5;
constexpr uint8_t MAC_ADDRESS_LENGTH = 6;
constexpr uint8_t PAYLOAD_SIZE = 9;
constexpr uint8_t NO_ID = 255;

// Message Sizes
constexpr uint8_t SIZE_OF[TOTAL_FRAMES] = {9, 2, 10, 2, 2}; // IM_HERE, START, SENSOR_DATA, ACK, SYNC

// Message Names
const char* const NAME[TOTAL_FRAMES] = {"IM_HERE", "START", "SENSOR_DATA", "ACK", "SYNC"};

// Message types enumeration (1 byte each)
typedef enum : uint8_t {
  IM_HERE     = 0x00, // Slave to Master: Indicates slave is present
  START       = 0x01, // Master to Slave: Instructs slave to start operations
  SENSOR_DATA = 0x02, // Slave to Master: Sends sensor data
  ACK         = 0x03, // Acknowledge message reception
  SYNC        = 0x04  // Slave to Master: Synchronize communication window timing
} MessageType;

// Sensor Data Structure
struct SensorData {
    uint8_t id;
    float temperature;
    float humidity;
} __attribute__((packed));

// Message Structure
struct Message {
    MessageType type;
    uint8_t payload[PAYLOAD_SIZE];
}__attribute__((packed));

#endif /* COMMON_H */
