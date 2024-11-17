/**
 * @file common.h 
 * @brief Shared Constants, Enums, and Structures for Master and Sensor Nodes
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

// Constants
constexpr uint8_t MAC_ADDRESS_LENGTH = 6;
constexpr uint8_t TOTAL_FRAMES = 2;
constexpr uint8_t PAYLOAD_MAX_SIZE = 9;

// Message names for debugging
const char* const NAME[TOTAL_FRAMES] = {"ACK", "TEMP_HUMID_DATA"};

// Message Types
enum class MessageType : uint8_t {
    ACK        = 0x00,     // Acknowledge message reception 
    TEMP_HUMID = 0x01,     // Slave to Master: Sends sensor data
};

// ACK Message Structure
struct AckMsg {     // ACK
    MessageType type;   
    MessageType acked_msg; // Acknowledged message type
} __attribute__((packed));

// Temperature and Humidity Data Structure
struct TempHumidMsg {
    MessageType type;   
    uint8_t room_id;
    float temperature;
    float humidity;
} __attribute__((packed));

#endif /* COMMON_H */