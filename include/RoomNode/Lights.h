/**
 * @file Lights.h
 * @brief Controls lights in the RoomNode, including brightness, color, and schedules.
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_TSL2591.h>
#include "config.h"
#include "Common/common.h"

// 433MHz bit sequences for sending commands
constexpr const char Light[] = "10001011100010111000101110111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";
constexpr const char LightRepeat[] = "100010001011100010111000101110111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";

constexpr const char MoreLight[] = "1110111011100010111011100010111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";
constexpr const char MoreRepeat[] = "101110111011100010111011100010111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";

constexpr const char LessLight[] = "10001000101110111011100010111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";
constexpr const char LessRepeat[] = "100010001000101110111011100010111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";

constexpr const char Yellow[] = "1110001000101110111011101110111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";
constexpr const char YellowRepeat[] = "101110001000101110111011101110111011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";

constexpr const char Blue[] = "1110111011101110001000101110001011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";
constexpr const char BlueRepeat[] = "101110111011101110001000101110001011100010111011100010111011100010001000101110111011100010111011100010001000100010111011101110111";

constexpr const char* COMMAND_NAMES[] = {"ON", "OFF", "MORE_LIGHT", "LESS_LIGHT", "BLUE", "YELLOW"};
constexpr const char* RESULT_NAMES[] = {"POSITIVE", "UNCLEAR", "NEGATIVE"};

// Commands for lights
enum class Command : uint8_t {
    ON,
    OFF,
    MORE_LIGHT,
    LESS_LIGHT,
    BLUE,
    YELLOW,
};

// Possible results after verifying a command
enum class CommandResult : uint8_t {
    POSITIVE,
    UNCLEAR,
    NEGATIVE,
};

class Lights {
public:
    Lights();
    Lights(const Lights&) = delete;
    Lights& operator=(const Lights&) = delete;

    // Determines initial on/off state using TSL2591 readings
    bool initializeState();

    // Sends a command and verifies its effect using TSL2591
    CommandResult sendCommand(Command command);

    // Checks if lights are currently on
    bool isOn() const;

    // Sets warm/cold time schedule
    void setSchedule(Time warm, Time cold);

    // Initializes warm/cold mode based on current time
    bool initializeMode(uint16_t current_minutes);

    // Checks if mode changed and updates lights accordingly
    void checkAndUpdateMode(uint16_t current_minutes);

    // Adjusts brightness within thresholds using MORE/LESS commands
    void adjustBrightness();

    // Check if there is enough ambient light
    bool isEnoughLight();

private:
    // Transmission parameters
    const uint8_t  MAX_INIT_RETRIES      = 3;
    const uint16_t DIGIT_DURATION        = 350;   // µs per bit
    const uint16_t PAUSE_US              = 8850;  // µs between repeats
    const uint8_t  NUM_REPEATS            = 5;
    const uint8_t  MAX_FAILURES           = 2;
    const float    LUX_MARGIN            = 20.0f; // Lux margin for verification
    const uint16_t VERIFY_DELAY_MS        = 1000;  // ms delay after command
    const float    DARK_THRESHOLD         = 50.0f; // Lux below which it's considered dark
    const float    BRIGHT_THRESHOLD       = 500.0f; // Lux above which it's considered bright
    const uint8_t  MAX_TRANSMIT_RETRIES   = 3;
    const uint8_t  COMMAND_REPEATS = 2;
    

    bool    is_on; 
    bool    warm_mode;
    Time warm;
    Time cold;
    bool    max_brightness;
    bool    min_brightness;

    // Mutex to protect RF transmitter
    SemaphoreHandle_t transmitterMutex;

    // Mutex for is_on shared variable
    SemaphoreHandle_t isOnMutex

    // TSL2591 sensor object
    Adafruit_TSL2591 tsl;

    // Sends a bit sequence to transmitter
    void transmit(const char *bits);

    // Sends a command with repeats
    bool sendSignal(const char *command, const char *repeat);

    // Sends a specific command (ON, OFF, MORE_LIGHT, etc.)
    bool send(Command command);

    // Initializes the TSL2591 sensor
    bool initTSL2591();

    // Gets a lux reading from TSL2591
    float getLuxValue();

    // Determines if current time falls into warm or cold mode
    bool determineMode(uint16_t current_minutes) const;
};
