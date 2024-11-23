/**
 * @file common.h
 * @brief Shared constants, enums, and structures for Master and Sensor Nodes
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#pragma once

#include <Arduino.h>
#include <time.h>

// Constants
constexpr uint8_t MAC_ADDRESS_LENGTH = 6;
constexpr uint8_t TOTAL_FRAMES = 4;

constexpr uint8_t ID_NOT_VALID = 255;

// Message names for debugging
const char* const MSG_NAME[TOTAL_FRAMES] = {"START", "ACK", "TEMP_HUMID_DATA", "NEW_POLLING_PERIOD"};

// Message Types
enum class MessageType : uint8_t {
    JOIN_NETWORK       = 0x00,     // SensorNode to MasterDevice: Request to join network
    ACK                = 0x01,     // Acknowledge message reception 
    TEMP_HUMID         = 0x02,     // SensorNode to MasterDevice: Sends sensor data
    NEW_POLLING_PERIOD = 0x03      // MasterDevice to SensorNode: Sends new sleep period
};

// ACK Message Structure
struct AckMsg {
    MessageType type = MessageType::ACK;   
    MessageType acked_msg; // Acknowledged message type
} __attribute__((packed));

// Temperature and Humidity Data Structure
struct TempHumidMsg {
    MessageType type = MessageType::TEMP_HUMID;   
    uint8_t room_id;
    float temperature;
    float humidity;
} __attribute__((packed));

struct NewPollingPeriodMsg {
    MessageType type = MessageType::NEW_POLLING_PERIOD;   
    uint32_t new_period_ms;
} __attribute__((packed));

struct JoinNetworkMsg {
    MessageType type = MessageType::JOIN_NETWORK;
    uint8_t id;
    uint32_t wake_interval_ms;
} __attribute__((packed));
