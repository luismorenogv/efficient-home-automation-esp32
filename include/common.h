/**
 * @file common.h
 * @brief Shared constants, enums, and structures for Master and Sensor Nodes
 *
 * @author Luis Moreno
 * @date Nov 24, 2024
 */

#pragma once

#include <Arduino.h>
#include <time.h>

// Constants
constexpr uint8_t MAC_ADDRESS_LENGTH = 6;

constexpr uint8_t MAX_WIFI_CHANNEL = 13;

constexpr uint8_t TOTAL_FRAMES = 4;

constexpr uint8_t ID_NOT_VALID = 255;

// Message names for debugging
const char* const MSG_NAME[TOTAL_FRAMES] = {"JOIN_NETWORK", "ACK", "TEMP_HUMID_DATA", "NEW_SLEEP_PERIOD"};

// Message Types
enum class MessageType : uint8_t {
    JOIN_NETWORK       = 0x00,     // SensorNode to MasterDevice: Request to join network
    ACK                = 0x01,     // Acknowledge message reception 
    TEMP_HUMID         = 0x02,     // SensorNode to MasterDevice: Sends sensor data
    NEW_SLEEP_PERIOD   = 0x03      // MasterDevice to SensorNode: Sends new sleep period
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

struct NewSleepPeriodMsg {
    MessageType type = MessageType::NEW_SLEEP_PERIOD;   
    uint32_t new_period_ms;
} __attribute__((packed));

struct JoinNetworkMsg {
    MessageType type = MessageType::JOIN_NETWORK;
    uint8_t room_id;
    uint32_t sleep_period_ms;
} __attribute__((packed));

// Define a union of all message types
union AllMessages {
    AckMsg ack;
    TempHumidMsg tempHumid;
    NewSleepPeriodMsg newSleep;
    JoinNetworkMsg join;
};

// Set MAX_MSG_SIZE to the size of the union
#define MAX_MSG_SIZE sizeof(union AllMessages)

// Incoming Message Structure for processing in FreeRTOS task
struct IncomingMsg {
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    uint8_t data[MAX_MSG_SIZE];
    uint32_t len;
} __attribute__((packed));
