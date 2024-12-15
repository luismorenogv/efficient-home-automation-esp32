/**
 * @file Lights.cpp
 * @brief Implementation of Lights class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/Lights.h"
#include <Arduino.h>

Lights::Lights() : is_on(false), transmitter_pin(TRANSMITTER_PIN), warm_mode(false) {
    pinMode(transmitter_pin, OUTPUT);
    digitalWrite(transmitter_pin, LOW);
    // Default schedules
    warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};
    cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
}

// Sends a bit sequence to the transmitter pin
void Lights::transmit(const char* bits) {
    for (size_t i = 0; i < strlen(bits); i++) {
        digitalWrite(transmitter_pin, (bits[i] == '1') ? HIGH : LOW);
        delayMicroseconds(DIGIT_DURATION);
    }
    digitalWrite(transmitter_pin, LOW);
}

// Sends a command followed by repeats
void Lights::sendSignal(const char* command, const char* repeat) {
    transmit(command);
    for (int i = 0; i < NUM_REPEATS; i++) {
        delayMicroseconds(PAUSE_MS);
        transmit(repeat);
    }
}

// Reads current LDR value
uint16_t Lights::readLDR(uint8_t ldr_pin) {
    return analogRead(ldr_pin);
}

// Attempts to determine initial lights state by comparing LDR before/after ON signal
bool Lights::initializeState(uint8_t ldr_pin) {
    LOG_INFO("Initializing Lights state...");
    for(uint8_t i = 0; i < MAX_INIT_RETRIES; i++){
        uint16_t initial_lux = readLDR(ldr_pin);
        LOG_INFO("Initial LDR: %u", initial_lux);
        
        sendSignal(Light, LightRepeat);
        delay(VERIFY_DELAY_MS);
        
        uint16_t new_lux = readLDR(ldr_pin);
        LOG_INFO("LDR after ON cmd: %u", new_lux);

        LOG_INFO("Initializing lights OFF");
        is_on = false;
        
        if (new_lux > initial_lux + LDR_MARGIN) {
            sendSignal(Light, LightRepeat);
        } else if (new_lux < initial_lux - LDR_MARGIN) {
        } else {
            LOG_WARNING("Unable to determine initial lights state in attempt %u", i);
            continue;
        }
        return true;
    }
    LOG_ERROR("Lights initialization failed");
    return false;
}

// Sends a command and verifies its effect using LDR readings
bool Lights::sendCommand(Command command) {
    LOG_INFO("Sending cmd: %u", static_cast<uint8_t>(command));
    uint16_t initial_lux = readLDR(LDR_PIN);
    LOG_INFO("LDR before cmd: %u", initial_lux);
    
    send(command);
    vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));
    
    uint16_t new_lux = readLDR(LDR_PIN);
    LOG_INFO("LDR after cmd: %u", new_lux);

    // Based on command, check if effect matches expectations
    bool expected_on = (command == Command::ON);
    bool expected_off = (command == Command::OFF);

    if (expected_on) {
        if (new_lux > initial_lux + LDR_MARGIN) {
            is_on = true;
            LOG_INFO("ON verified.");
            return true;
        }
        LOG_WARNING("Failed to verify ON.");
        return false;
    } else if (expected_off) {
        if (new_lux < initial_lux - LDR_MARGIN) {
            is_on = false;
            LOG_INFO("OFF verified.");
            return true;
        }
        LOG_WARNING("Failed to verify OFF.");
        return false;
    } else if (command == Command::MORE_LIGHT) {
        if (new_lux > initial_lux) {
            LOG_INFO("Brightness up verified.");
            return true;
        }
        LOG_WARNING("Failed to verify MORE_LIGHT.");
        return false;
    } else if (command == Command::LESS_LIGHT) {
        if (new_lux < initial_lux) {
            LOG_INFO("Brightness down verified.");
            return true;
        }
        LOG_WARNING("Failed to verify LESS_LIGHT.");
        return false;
    } else if (command == Command::BLUE || command == Command::YELLOW) {
        // Color changes assumed successful
        LOG_INFO("Color change command sent.");
        return true;
    }

    LOG_WARNING("Unknown command verification failed.");
    return false;
}

bool Lights::isOn() const {
    return is_on;
}

// Sets a new warm/cold schedule, ensuring they differ
void Lights::setSchedule(Time w, Time c) {
    if (w.hour == c.hour && w.min == c.min) {
        LOG_WARNING("Invalid schedule: warm/cold times identical.");
        return;
    }
    warm = w;
    cold = c;
    LOG_INFO("Schedule: Warm %02u:%02u, Cold %02u:%02u", warm.hour, warm.min, cold.hour, cold.min);
}

// Determines if current time falls into warm or cold mode
bool Lights::determineMode(uint16_t current_minutes) const {
    uint16_t warm_minutes = warm.hour * 60 + warm.min;
    uint16_t cold_minutes = cold.hour * 60 + cold.min;

    LOG_INFO("determineMode function called with:\r\nwarm_minutes: %u\r\ncold_minutes: %u\r\ncurrent_minutes: %u", warm_minutes, cold_minutes, current_minutes);

    if (warm_minutes < cold_minutes) {
        return (current_minutes >= warm_minutes && current_minutes < cold_minutes);
    } else {
        return (current_minutes >= warm_minutes || current_minutes < cold_minutes);
    }
}

// Initializes mode (warm/cold) based on current time and sends corresponding color command
bool Lights::initializeMode(uint16_t current_minutes) {
    warm_mode = determineMode(current_minutes);
    LOG_INFO("Initial mode: %s", warm_mode ? "WARM" : "COLD");
    return warm_mode ? sendCommand(Command::YELLOW) : sendCommand(Command::BLUE);
}

// Checks if mode changed and updates color if needed
void Lights::checkAndUpdateMode(uint16_t current_minutes) {
    bool new_mode = determineMode(current_minutes);
    LOG_INFO("Current mode: %s", new_mode ? "WARM" : "COLD");
    if (new_mode != warm_mode) {
        warm_mode = new_mode;
        LOG_INFO("Mode changed to %s", warm_mode ? "WARM" : "COLD");
        warm_mode ? sendCommand(Command::YELLOW) : sendCommand(Command::BLUE);
    }
}

// Adjusts brightness to keep LDR within thresholds, sending MORE/LESS commands as needed
void Lights::adjustBrightness(uint8_t ldr_pin) {
    uint16_t current_lux = readLDR(ldr_pin);
    LOG_INFO("Current LDR: %u", current_lux);

    if (current_lux < DARK_THRESHOLD) {
        LOG_INFO("Increasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) < DARK_THRESHOLD) {
            if (!sendCommand(Command::MORE_LIGHT)) {
                failures++;
                if (failures >= MAX_FAILURES) {
                    LOG_INFO("Max brightness reached.");
                    break;
                }
            }
        }
    } else if (current_lux > BRIGHT_THRESHOLD) {
        LOG_INFO("Decreasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) > BRIGHT_THRESHOLD) {
            if (!sendCommand(Command::LESS_LIGHT)) {
                failures++;
                if (failures >= MAX_FAILURES) {
                    LOG_INFO("Min brightness reached.");
                    break;
                }
            }
        }
    }
}

// Sends a predefined signal sequence based on the command
void Lights::send(Command command) {
    switch(command) {
        case Command::ON:
        case Command::OFF:
            sendSignal(Light, LightRepeat);
            break;
        case Command::MORE_LIGHT:
            sendSignal(MoreLight, MoreRepeat);
            break;
        case Command::LESS_LIGHT:
            sendSignal(LessLight, LessRepeat);
            break;
        case Command::BLUE:
            sendSignal(Blue, BlueRepeat);
            break;
        case Command::YELLOW:
            sendSignal(Yellow, YellowRepeat);
            break;
        default:
            LOG_WARNING("Unknown command");
            break;
    }
}