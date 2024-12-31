/**
 * @file Lights.cpp
 * @brief Implementation of Lights class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/Lights.h"
#include <Arduino.h>

Lights::Lights() : is_on(false), transmitter_pin(TRANSMITTER_PIN), warm_mode(false),
                   max_brightness(false), min_brightness(false) {
    pinMode(transmitter_pin, OUTPUT);
    digitalWrite(transmitter_pin, LOW);
    // Default schedules
    warm = {DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM};
    cold = {DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD};

    transmitterMutex = xSemaphoreCreateMutex();
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
bool Lights::sendSignal(const char* command, const char* repeat) {
    // Attempt to take the mutex with a timeout to prevent deadlocks
    if (xSemaphoreTake(transmitterMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        // Successfully acquired the mutex
        transmit(command);
    for (int i = 0; i < NUM_REPEATS; i++) {
        uint8_t time_ms = PAUSE_US/1000;
        uint8_t remaining_time_us = PAUSE_US % 1000;
        vTaskDelay(pdMS_TO_TICKS(time_ms));
        delayMicroseconds(remaining_time_us);
        transmit(repeat);
    }
        
        // Release the mutex after transmission
        xSemaphoreGive(transmitterMutex);
    } else {
        // Failed to acquire the mutex within the timeout
        LOG_WARNING("Failed to acquire transmitter mutex. Command not sent.");
        return false;
    }

    return true;
    
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
        
        send(Command::ON);
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
CommandResult Lights::sendCommand(Command command) {
    CommandResult result = CommandResult::UNCLEAR;
    LOG_INFO("Sending cmd: %u", static_cast<uint8_t>(command));
    uint16_t initial_lux = readLDR(LDR_PIN);
    LOG_INFO("LDR before cmd: %u", initial_lux);
    
    send(command);
    vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));
    
    uint16_t new_lux = readLDR(LDR_PIN);
    LOG_INFO("LDR after cmd: %u", new_lux);

    // Verify result according to command, initial_lux and new_lux
    if (command == Command::MORE_LIGHT || command == Command::ON){
        if(new_lux > initial_lux + LDR_MARGIN){
            result = CommandResult::POSITIVE;
            if (command == Command::ON){
                is_on = true;
                LOG_INFO("Lights are ON");
            }
        } else if (new_lux < initial_lux - LDR_MARGIN){
            result = CommandResult::NEGATIVE;
        } else{
            result = CommandResult::UNCLEAR;
        }
    } else if (command == Command::LESS_LIGHT || command == Command::OFF){
        if(new_lux < initial_lux - LDR_MARGIN){
            result = CommandResult::POSITIVE;
            if (command == Command::OFF){
                is_on = false;
                LOG_INFO("Lights are OFF");
            }
        } else if (new_lux > initial_lux + LDR_MARGIN){
            result = CommandResult::NEGATIVE;
        } else{
            result = CommandResult::UNCLEAR;
        }
    } else {
        // Color change command is assumed as positive
        result = CommandResult::POSITIVE;
    }
    LOG_INFO("Command %s resulted %s", COMMAND_NAMES[static_cast<uint8_t>(command)], RESULT_NAMES[static_cast<uint8_t>(result)]);
    return result;
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
    if (warm_mode){
        sendCommand(Command::YELLOW);
    } else {
        sendCommand(Command::BLUE);
    }
    return true;
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
        if (!max_brightness){
            min_brightness = false;
            LOG_INFO("Increasing brightness.");
            uint8_t failures = 0;
            while(readLDR(ldr_pin) < DARK_THRESHOLD) {
                if (sendCommand(Command::MORE_LIGHT) != CommandResult::POSITIVE) {
                    failures++;
                    if (failures >= MAX_FAILURES) {
                        LOG_INFO("Max brightness reached.");
                        break;
                    }
                }
            }
        }
    } else if (current_lux > BRIGHT_THRESHOLD) {
        if (!min_brightness){
            max_brightness = false;
            LOG_INFO("Decreasing brightness.");
            uint8_t failures = 0;
            while(readLDR(ldr_pin) > BRIGHT_THRESHOLD) {
                if (sendCommand(Command::LESS_LIGHT) != CommandResult::POSITIVE) {
                    failures++;
                    if (failures >= MAX_FAILURES) {
                        LOG_INFO("Min brightness reached.");
                        break;
                    }
                }
            }
        }
    }
}

bool Lights::isEnoughLight(uint8_t ldr_pin){
    uint16_t current_lux = readLDR(ldr_pin);
    LOG_INFO("Current light: %u", current_lux);
    if (current_lux >= DARK_THRESHOLD){
        return true;
    }
    return false;
}

// Sends a predefined signal sequence based on the command
bool Lights::send(Command command) {
    bool success = false;
    for (uint8_t i = 0; i < MAX_TRANSMIT_RETRIES; i++){
        switch(command) {
            case Command::ON:
            case Command::OFF:
                success = sendSignal(Light, LightRepeat);
                break;
            case Command::MORE_LIGHT:
                success = sendSignal(MoreLight, MoreRepeat);
                break;
            case Command::LESS_LIGHT:
                success = sendSignal(LessLight, LessRepeat);
                break;
            case Command::BLUE:
                success = sendSignal(Blue, BlueRepeat);
                break;
            case Command::YELLOW:
                success = sendSignal(Yellow, YellowRepeat);
                break;
            default:
                LOG_WARNING("Unknown command");
                break;
        }

        if (success){
            return success;
        }
    }
    return false;
}