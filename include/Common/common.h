/**
 * @file common.h
 * @brief Shared constants, enums, and structures for Master, RoomNode and MasterNodes
 * 
 * 
 * @author Luis Moreno
 * @date Nov 24, 2024
 */

#pragma once

#include <Arduino.h>

constexpr uint8_t MAC_ADDRESS_LENGTH = 6;
constexpr uint8_t MAX_WIFI_CHANNEL = 13;
constexpr uint8_t TOTAL_FRAMES = 6;
constexpr uint8_t ID_NOT_VALID = 255;

// Names of message types for debugging
const char* const MSG_NAME[TOTAL_FRAMES] = {
    "JOIN_SENSOR", "JOIN_ROOM", "ACK", "TEMP_HUMID_DATA", "NEW_SLEEP_PERIOD", "NEW_TIME"
};

enum class MessageType : uint8_t {
    JOIN_SENSOR      = 0x00,
    JOIN_ROOM        = 0x01,
    ACK              = 0x02, 
    TEMP_HUMID       = 0x03,
    NEW_SLEEP_PERIOD = 0x04,
    NEW_SCHEDULE     = 0x05,
};

enum class NodeType : uint8_t {
    SENSOR = 0x00,
    ROOM   = 0x01,
};

enum class TimeType : uint8_t {
    WARM = 0x00,
    COLD = 0x01,
};

// Simple ACK message linking an ACK to a previous message
struct AckMsg {
    MessageType type = MessageType::ACK;   
    MessageType acked_msg; 
} __attribute__((packed));

// Holds temperature/humidity data for a room
struct TempHumidMsg {
    MessageType type = MessageType::TEMP_HUMID;   
    uint8_t room_id;
    float temperature;
    float humidity;
} __attribute__((packed));

// Holds updated sleep period data
struct NewSleepPeriodMsg {
    MessageType type = MessageType::NEW_SLEEP_PERIOD;   
    uint32_t new_period_ms;
} __attribute__((packed));

// Holds sensor join request data
struct JoinSensorMsg {
    MessageType type = MessageType::JOIN_SENSOR;
    uint8_t room_id;
    uint32_t sleep_period_ms;
} __attribute__((packed));

// Basic time structure for daily schedules
struct Time {
    uint8_t hour;
    uint8_t min;
};

// Holds room join request data, including warm/cold times
struct JoinRoomMsg {
    MessageType type = MessageType::JOIN_ROOM;
    uint8_t room_id;
    Time warm;
    Time cold;
} __attribute__((packed));

// Holds new schedule data
struct NewScheduleMsg {
    MessageType type = MessageType::NEW_SCHEDULE;   
    Time cold;
    Time warm;
} __attribute__((packed));

// Union of all message types
union AllMessages {
    AckMsg ack;
    TempHumidMsg temp_humid;
    NewSleepPeriodMsg new_sleep;
    JoinSensorMsg join_sensor;
    JoinRoomMsg join_room;
    NewScheduleMsg new_schedule;
};

#define MAX_MSG_SIZE sizeof(union AllMessages)

// Generic incoming message structure for queue handling
struct IncomingMsg {
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    uint8_t data[MAX_MSG_SIZE];
    uint32_t len;
} __attribute__((packed));