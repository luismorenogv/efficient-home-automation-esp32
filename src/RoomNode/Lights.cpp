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
    Serial.println("Initializing Lights state...");
    
    uint16_t initial_lux = readLDR(ldr_pin);
    Serial.printf("Initial LDR: %u\r\n", initial_lux);
    
    sendSignal(Light, LightRepeat);
    delay(VERIFY_DELAY_MS);
    
    uint16_t new_lux = readLDR(ldr_pin);
    Serial.printf("LDR after ON cmd: %u\r\n", new_lux);

    Serial.println("Initializing lights OFF");
    is_on = false;
    
    if (new_lux > initial_lux + LDR_MARGIN) {
        sendSignal(Light, LightRepeat);
    } else if (new_lux < initial_lux - LDR_MARGIN) {
    } else {
        Serial.println("Unable to determine initial lights state.");
        return false;
    }
    return true;
}

// Sends a command and verifies its effect using LDR readings
bool Lights::sendCommand(Command command) {
    Serial.printf("Sending cmd: %u\r\n", static_cast<uint8_t>(command));
    uint16_t initial_lux = readLDR(LDR_PIN);
    Serial.printf("LDR before cmd: %u\r\n", initial_lux);
    
    send(command);
    vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));
    
    uint16_t new_lux = readLDR(LDR_PIN);
    Serial.printf("LDR after cmd: %u\r\n", new_lux);

    // Based on command, check if effect matches expectations
    bool expected_on = (command == Command::ON);
    bool expected_off = (command == Command::OFF);

    if (expected_on) {
        if (new_lux > initial_lux + LDR_MARGIN) {
            is_on = true;
            Serial.println("ON verified.");
            return true;
        }
        Serial.println("Failed to verify ON.");
        return false;
    } else if (expected_off) {
        if (new_lux < initial_lux - LDR_MARGIN) {
            is_on = false;
            Serial.println("OFF verified.");
            return true;
        }
        Serial.println("Failed to verify OFF.");
        return false;
    } else if (command == Command::MORE_LIGHT) {
        if (new_lux > initial_lux) {
            Serial.println("Brightness up verified.");
            return true;
        }
        Serial.println("Failed to verify MORE_LIGHT.");
        return false;
    } else if (command == Command::LESS_LIGHT) {
        if (new_lux < initial_lux) {
            Serial.println("Brightness down verified.");
            return true;
        }
        Serial.println("Failed to verify LESS_LIGHT.");
        return false;
    } else if (command == Command::BLUE || command == Command::YELLOW) {
        // Color changes assumed successful
        Serial.println("Color change command sent.");
        return true;
    }

    Serial.println("Unknown command verification failed.");
    return false;
}

bool Lights::isOn() const {
    return is_on;
}

// Sets a new warm/cold schedule, ensuring they differ
void Lights::setSchedule(Time w, Time c) {
    if (w.hour == c.hour && w.min == c.min) {
        Serial.println("Invalid schedule: warm/cold times identical.");
        return;
    }
    warm = w;
    cold = c;
    Serial.printf("Schedule: Warm %02u:%02u, Cold %02u:%02u\r\n", warm.hour, warm.min, cold.hour, cold.min);
}

// Determines if current time falls into warm or cold mode
bool Lights::determineMode(uint16_t current_minutes) const {
    uint16_t warm_minutes = warm.hour * 60 + warm.min;
    uint16_t cold_minutes = cold.hour * 60 + cold.min;

    Serial.printf("determineMode function called with:\r\nwarm_minutes: %u\r\ncold_minutes: %u\r\ncurrent_minutes: %u\r\n", warm_minutes, cold_minutes, current_minutes);

    if (warm_minutes < cold_minutes) {
        return (current_minutes >= warm_minutes && current_minutes < cold_minutes);
    } else {
        return (current_minutes >= warm_minutes || current_minutes < cold_minutes);
    }
}

// Initializes mode (warm/cold) based on current time and sends corresponding color command
bool Lights::initializeMode(uint16_t current_minutes) {
    warm_mode = determineMode(current_minutes);
    Serial.printf("Initial mode: %s\r\n", warm_mode ? "WARM" : "COLD");
    return warm_mode ? sendCommand(Command::YELLOW) : sendCommand(Command::BLUE);
}

// Checks if mode changed and updates color if needed
void Lights::checkAndUpdateMode(uint16_t current_minutes) {
    bool new_mode = determineMode(current_minutes);
    Serial.printf("Current mode: %s\r\n", new_mode ? "WARM" : "COLD");
    if (new_mode != warm_mode) {
        warm_mode = new_mode;
        Serial.printf("Mode changed to %s\r\n", warm_mode ? "WARM" : "COLD");
        warm_mode ? sendCommand(Command::YELLOW) : sendCommand(Command::BLUE);
    }
}

// Adjusts brightness to keep LDR within thresholds, sending MORE/LESS commands as needed
void Lights::adjustBrightness(uint8_t ldr_pin) {
    uint16_t current_lux = readLDR(ldr_pin);
    Serial.printf("Current LDR: %u\r\n", current_lux);

    if (current_lux < DARK_THRESHOLD) {
        Serial.println("Increasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) < DARK_THRESHOLD) {
            if (!sendCommand(Command::MORE_LIGHT)) {
                failures++;
                if (failures >= MAX_FAILURES) {
                    Serial.println("Max brightness reached.");
                    break;
                }
            }
        }
    } else if (current_lux > BRIGHT_THRESHOLD) {
        Serial.println("Decreasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) > BRIGHT_THRESHOLD) {
            if (!sendCommand(Command::LESS_LIGHT)) {
                failures++;
                if (failures >= MAX_FAILURES) {
                    Serial.println("Min brightness reached.");
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
            Serial.println("Unknown command");
            break;
    }
}