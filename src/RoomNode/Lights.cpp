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
    // Set default schedules
    warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};
    cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};
}

void Lights::transmit(const char* bits) {
    for (size_t i = 0; i < strlen(bits); i++) {
        digitalWrite(transmitter_pin, (bits[i] == '1') ? HIGH : LOW);
        delayMicroseconds(DIGIT_DURATION);
    }
    digitalWrite(transmitter_pin, LOW);
}

void Lights::sendSignal(const char *command, const char *repeat){
    transmit(command);
    for (int i = 0; i < NUM_REPEATS; i++) {
        delayMicroseconds(PAUSE_MS);
        transmit(repeat);
    }
}

uint16_t Lights::readLDR(uint8_t ldr_pin) {
    return analogRead(ldr_pin);
}

bool Lights::initializeState(uint8_t ldr_pin){
    Serial.println("Initializing Lights state...");
    
    uint16_t initial_lux = readLDR(ldr_pin);
    Serial.printf("Initial LDR reading: %u\n", initial_lux);
    
    sendSignal(Light, LightRepeat);
    delay(VERIFY_DELAY_MS);
    
    uint16_t new_lux = readLDR(ldr_pin);
    Serial.printf("New LDR reading after ON command: %u\n", new_lux);
    
    if (new_lux > initial_lux + LDR_MARGIN) {
        is_on = true;
        Serial.println("Lights are ON.");
    }
    else if (new_lux < initial_lux - LDR_MARGIN) {
        is_on = false;
        Serial.println("Lights are OFF after sending ON command (unexpected).");
    }
    else {
        Serial.println("Unable to determine initial lights state.");
        return false;
    }
    //Return to initial state;
    sendSignal(Light, LightRepeat);
    return true;
}

bool Lights::sendCommand(Command command){
    Serial.printf("Sending command: %u\n", static_cast<uint8_t>(command));
    uint16_t initial_lux = readLDR(LDR_PIN);
    Serial.printf("LDR before command: %u\n", initial_lux);
    
    send(command);
    vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));
    
    uint16_t new_lux = readLDR(LDR_PIN);
    Serial.printf("LDR after command: %u\n", new_lux);
    
    bool expected_on = (command == Command::ON);
    bool expected_off = (command == Command::OFF);

    if (expected_on) {
        if (new_lux > initial_lux + LDR_MARGIN) {
            is_on = true;
            Serial.println("Lights ON verified.");
            return true;
        }
        Serial.println("Failed to verify Lights ON.");
        return false;
    } else if (expected_off) {
        if (new_lux < initial_lux - LDR_MARGIN) {
            is_on = false;
            Serial.println("Lights OFF verified.");
            return true;
        }
        Serial.println("Failed to verify Lights OFF.");
        return false;
    } else if (command == Command::MORE_LIGHT) {
        if (new_lux > initial_lux) {
            Serial.println("Increased brightness verified.");
            return true;
        }
        Serial.println("Failed to verify MORE LIGHT.");
        return false;
    } else if (command == Command::LESS_LIGHT) {
        if (new_lux < initial_lux) {
            Serial.println("Decreased brightness verified.");
            return true;
        }
        Serial.println("Failed to verify LESS LIGHT.");
        return false;
    } else if (command == Command::BLUE || command == Command::YELLOW) {
        // For color changes successis assumed as we have no direct LDR feedback
        Serial.println("Color mode change command sent.");
        return true;
    }

    Serial.println("Unknown command. Verification failed.");
    return false;
}

bool Lights::isOn() const {
    return is_on;
}

void Lights::setSchedule(Time w, Time c) {
    if((w.hour == c.hour) && (w.min == c.min)){
        Serial.println("Invalid schedule: Warm and Cold times are identical.");
        return;
    }
    warm = w;
    cold = c;
    Serial.printf("Schedule set: Warm %02u:%02u, Cold %02u:%02u\n", warm.hour, warm.min, cold.hour, cold.min);
}

bool Lights::determineMode(uint16_t current_minutes) const {
    uint16_t warm_minutes = warm.hour * 60 + warm.min;
    uint16_t cold_minutes = cold.hour * 60 + cold.min;

    // If warm_minutes < cold_minutes:
    // warm_mode if current >= warm_minutes && current < cold_minutes
    // else cold_mode
    // If schedule crosses midnight (warm > cold):
    // warm_mode if current_minutes >= warm_minutes OR current_minutes < cold_minutes
    if (warm_minutes < cold_minutes) {
        return (current_minutes >= warm_minutes && current_minutes < cold_minutes);
    } else {
        // crosses midnight
        return (current_minutes >= warm_minutes || current_minutes < cold_minutes);
    }
}

bool Lights::initializeMode(uint16_t current_minutes) {
    warm_mode = determineMode(current_minutes);
    Serial.printf("Initial mode: %s\n", warm_mode ? "WARM" : "COLD");
    // Switch to correct mode color
    if (warm_mode) {
        return sendCommand(Command::YELLOW);
    } else {
        return sendCommand(Command::BLUE);
    }
}

void Lights::checkAndUpdateMode(uint16_t current_minutes) {
    bool new_mode = determineMode(current_minutes);
    if (new_mode != warm_mode) {
        warm_mode = new_mode;
        Serial.printf("Mode changed to %s\n", warm_mode ? "WARM" : "COLD");
        if (warm_mode) {
            sendCommand(Command::YELLOW);
        } else {
            sendCommand(Command::BLUE);
        }
    }
}

void Lights::adjustBrightness(uint8_t ldr_pin) {
    uint16_t current_lux = readLDR(ldr_pin);
    Serial.printf("Current LDR: %u\n", current_lux);

    if (current_lux < DARK_THRESHOLD) {
        Serial.println("Too dim, increasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) < DARK_THRESHOLD){
            if (!sendCommand(Command::MORE_LIGHT)){
                failures++;
                if (failures >= MAX_FAILURES){
                    Serial.println("Maximum brightness reached.");
                    break;
                }
            }

        }
    }
    else if (current_lux > BRIGHT_THRESHOLD) {
        Serial.println("Too bright, decreasing brightness.");
        uint8_t failures = 0;
        while(readLDR(ldr_pin) > BRIGHT_THRESHOLD){
            if (!sendCommand(Command::LESS_LIGHT)){
                failures++;
                if (failures >= MAX_FAILURES){
                    Serial.println("Minimum brightness reached.");
                    break;
                }
            }

        }
    }
}

void Lights::send(Command command){
    switch(command){
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
