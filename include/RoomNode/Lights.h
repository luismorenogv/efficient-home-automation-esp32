/**
 * @file Lights.h
 * @brief Declaration of Lights class for managing the Lights with RF OOK signals
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#pragma once
#include <Arduino.h>
#include "config.h"
#include "Common/common.h"

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

enum class Command : uint8_t {
    ON,
    OFF,
    MORE_LIGHT,
    LESS_LIGHT,
    BLUE,
    YELLOW,
};

class Lights {
public:
    Lights();
    Lights(const Lights&) = delete;
    Lights& operator=(const Lights&) = delete;

    bool initializeState(uint8_t ldr_pin);
    bool sendCommand(Command command);
    bool isOn() const;
    void setSchedule(Time warm, Time cold);
    bool initializeMode(uint16_t current_minutes);
    void checkAndUpdateMode(uint16_t current_minutes);
    void adjustBrightness(uint8_t ldr_pin);

private:
    const uint16_t DIGIT_DURATION = 350; // in microseconds
    const uint16_t PAUSE_MS = 8850;      // in microseconds
    const uint8_t NUM_REPEATS = 5;
    const uint8_t MAX_FAILURES = 2;
    const uint16_t LDR_MARGIN = 100;     
    const uint16_t VERIFY_DELAY_MS = 1000; 
    const uint16_t DARK_THRESHOLD = 2000;
    const uint16_t BRIGHT_THRESHOLD = 3000;

    bool is_on; 
    uint8_t transmitter_pin;

    Time warm;
    Time cold;
    bool warm_mode; // track whether currently in warm mode or cold mode

    void transmit(const char *bits);
    void sendSignal(const char *command, const char *repeat);
    uint16_t readLDR(uint8_t ldr_pin);
    void send(Command command);

    bool determineMode(uint16_t current_minutes) const;
};
