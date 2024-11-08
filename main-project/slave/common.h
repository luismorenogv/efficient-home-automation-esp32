/**
 * @file common.h
 * @brief Common definitions and structures for master and slave devices.
 *
 * @author Luis Moreno
 * @date November 8, 2024
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// Constants
constexpr uint8_t TOTAL_FRAMES = 6;
constexpr uint8_t MAC_ADDRESS_LENGTH = 6;
constexpr uint8_t PAYLOAD_SIZE = 9;
constexpr uint8_t NO_ID = 255;

constexpr uint16_t MSG_TIMEOUT = 1000;

// Message Names
const char* const NAME[TOTAL_FRAMES] = {"IM_HERE", "START", "SENSOR_DATA", "ACK", "REQ", "ACTION"};


enum class MessageType : uint8_t {
  IM_HERE     = 0x00, // Slave to Master: Indicates slave is present
  START       = 0x01, // Master to Slave: Instructs slave to start operations
  SENSOR_DATA = 0x02, // Slave to Master: Sends sensor data
  ACK         = 0x03, // Acknowledge message reception
  REQ         = 0x04, // Slave to Master: Request actions from master
  ACTION      = 0x05  // Master to Slave: Send actions to slave
};


enum class ActionCode : uint8_t {
  TURN_OFF_AC = 0x00,
  TURN_ON_AC  = 0x01
};


struct SensorData {
    uint8_t id;           // ID of the slave sending the data
    float temperature;    // Temperature value
    float humidity;       // Humidity value
} __attribute__((packed));

static_assert(sizeof(SensorData) == 9, "SensorData size mismatch");


struct ActionMessage {
    uint8_t num_actions;                        // Number of actions
    uint8_t actions[PAYLOAD_SIZE - 1];          // Array of action codes
} __attribute__((packed));


struct Message {
    MessageType type;                           // Type of the message
    uint8_t payload[PAYLOAD_SIZE];              // Payload of the message
} __attribute__((packed));

// Message Sizes
constexpr uint8_t SIZE_OF[TOTAL_FRAMES] = {
    sizeof(MessageType) + 2 * sizeof(uint32_t),    // IM_HERE (type + wake_up_period_ms + time_awake_ms)
    sizeof(MessageType) + sizeof(uint8_t),         // START (type + id)
    sizeof(MessageType) + sizeof(SensorData),      // SENSOR_DATA (type + SensorData)
    sizeof(MessageType) + sizeof(MessageType),     // ACK (type + acked_msg_type)
    sizeof(MessageType) + sizeof(uint8_t),         // REQ (type + id)
    sizeof(MessageType) + sizeof(ActionMessage)    // ACTION (type + ActionMessage)
};

#endif /* COMMON_H */


