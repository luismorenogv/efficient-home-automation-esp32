/**
 * @file Lights.cpp
 * @brief Implementation of Lights class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/Lights.h"
#include <Arduino.h>
#include <Wire.h>

Lights::Lights() 
    : is_on(false),
      warm_mode(false),
      max_brightness(false),
      min_brightness(false),
      warm({DEFAULT_HOUR_WARM, DEFAULT_MIN_WARM}),
      cold({DEFAULT_HOUR_COLD, DEFAULT_MIN_COLD}),
      tsl(Adafruit_TSL2591(2591))
{
    pinMode(TRANSMITTER_PIN, OUTPUT);
    digitalWrite(TRANSMITTER_PIN, LOW);

    // Initialize mutex
    transmitterMutex = xSemaphoreCreateMutex();
    lightsSensorMutex = xSemaphoreCreateMutex();
    isOnMutex = xSemaphoreCreateMutex();
}

bool Lights::initializeState() {
    LOG_INFO("Initializing Lights state...");

    // Initialize I2C with predefined SDA and SCL pins
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize TSL2591
    if (!initTSL2591()) {
        LOG_ERROR("TSL2591 initialization failed.");
        return false;
    }

    // Attempt to deduce initial lights state
    for(uint8_t i = 0; i < MAX_INIT_RETRIES; i++) {
        float initial_lux = getLuxValue();
        LOG_INFO("Initial lux: %.2f", initial_lux);

        // Send an ON command (same as OFF)
        send(Command::ON);
        vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));

        float new_lux = getLuxValue();
        LOG_INFO("Lux after ON cmd: %.2f", new_lux);

        LOG_INFO("Initializing lights OFF");
        if (xSemaphoreTake(isOnMutex, pdMS_TO_TICKS(100)) == pdTRUE) { // 100ms timeout
            is_on = false;
            xSemaphoreGive(isOnMutex);
        } else {
            return false;
            LOG_WARNING("Failed to acquire isOnMutex in initializeState()");
        }

        if (new_lux > initial_lux + LUX_MARGIN) {
            // If lux increased, assume lights turned on
            // Turn them back off
            if (xSemaphoreTake(isOnMutex, pdMS_TO_TICKS(100)) == pdTRUE) { // 100ms timeout
                is_on = true;
                xSemaphoreGive(isOnMutex);
            } else {
                return false;
                LOG_WARNING("Failed to acquire isOnMutex in initializeState()");
            }
            if (sendCommand(Command::OFF) != CommandResult::POSITIVE) {
                LOG_ERROR("Failed to turn lights back OFF");
                continue;
            }
            LOG_INFO("Lights turned back OFF");
        }
        else if (new_lux < initial_lux - LUX_MARGIN) {
            // Leave lights off
            LOG_INFO("No additional signal is needed. Lights are OFF");
        }
        else {
            LOG_WARNING("Unable to determine initial lights state in attempt %u", i);
            continue;
        }

        return true;
    }

    LOG_ERROR("Lights initialization failed after multiple attempts");
    return false;
}

CommandResult Lights::sendCommand(Command command) {
    CommandResult result = CommandResult::UNCLEAR;
    // Attempt to take the mutex with a timeout to prevent deadlocks
    if (xSemaphoreTake(transmitterMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Successfully acquired the mutex
        for (uint8_t i ; i < COMMAND_REPEATS; i++){
            LOG_INFO("Sending cmd: %u (attempt %u/%u)", static_cast<uint8_t>(command), i, COMMAND_REPEATS);

            // Read initial lux
            float initial_lux = getLuxValue();
            LOG_INFO("Lux before cmd: %.2f", initial_lux);

            
            // Send the command
            send(command);

            // Wait for verification
            vTaskDelay(pdMS_TO_TICKS(VERIFY_DELAY_MS));

            // Read new lux
            float new_lux = getLuxValue();
            LOG_INFO("Lux after cmd: %.2f", new_lux);

            // Verify result according to command, initial_lux, and new_lux
            if (command == Command::MORE_LIGHT || command == Command::ON) {
                if(new_lux > initial_lux + LUX_MARGIN){
                    result = CommandResult::POSITIVE;
                    if (command == Command::ON){
                        xSemaphoreTake(isOnMutex, portMAX_DELAY);
                            is_on = true;
                        xSemaphoreGive(isOnMutex);
                        LOG_INFO("Lights are ON");
                    }
                }
                else if (new_lux < initial_lux - LUX_MARGIN){
                    result = CommandResult::NEGATIVE;
                }
                else{
                    result = CommandResult::UNCLEAR;
                }
            }
            else if (command == Command::LESS_LIGHT || command == Command::OFF){
                if(new_lux < initial_lux - LUX_MARGIN){
                    result = CommandResult::POSITIVE;
                    if (command == Command::OFF){
                        xSemaphoreTake(isOnMutex, portMAX_DELAY);
                        is_on = false;
                        xSemaphoreGive(isOnMutex);
                        LOG_INFO("Lights are OFF");
                    }
                }
                else if (new_lux > initial_lux + LUX_MARGIN){
                    result = CommandResult::NEGATIVE;
                }
                else{
                    result = CommandResult::UNCLEAR;
                }
            }
            else {
                // Color change commands are assumed as positive
                result = CommandResult::POSITIVE;
            }

            LOG_INFO("Command %s resulted %s", COMMAND_NAMES[static_cast<uint8_t>(command)], RESULT_NAMES[static_cast<uint8_t>(result)]);
            if (result == CommandResult::POSITIVE || result == CommandResult::NEGATIVE){
                break;
            }
        }
    }
    else {
        // Failed to acquire the mutex within the timeout
        LOG_WARNING("Failed to acquire transmitter mutex. Command not sent.");
    }
    return result;
}

bool Lights::isOn() const {
    bool is_on_return = false;
    if (xSemaphoreTake(isOnMutex, pdMS_TO_TICKS(100)) == pdTRUE) { // 100ms timeout
        is_on_return = is_on;
        xSemaphoreGive(isOnMutex);
    } else {
        LOG_WARNING("Failed to acquire isOnMutex in isOn()");
    }
    return is_on_return;
}

void Lights::setSchedule(Time w, Time c) {
    if (w.hour == c.hour && w.min == c.min) {
        LOG_WARNING("Invalid schedule: warm/cold times identical.");
        return;
    }
    warm = w;
    cold = c;
    LOG_INFO("Schedule: Warm %02u:%02u, Cold %02u:%02u", warm.hour, warm.min, cold.hour, cold.min);
}

bool Lights::determineMode(uint16_t current_minutes) const {
    uint16_t warm_minutes = warm.hour * 60 + warm.min;
    uint16_t cold_minutes = cold.hour * 60 + cold.min;

    LOG_INFO("determineMode function called with:\r\nwarm_minutes: %u\r\ncold_minutes: %u\r\ncurrent_minutes: %u", warm_minutes, cold_minutes, current_minutes);

    if (warm_minutes < cold_minutes) {
        return (current_minutes >= warm_minutes && current_minutes < cold_minutes);
    }
    else {
        return (current_minutes >= warm_minutes || current_minutes < cold_minutes);
    }
}

bool Lights::initializeMode(uint16_t current_minutes) {
    warm_mode = determineMode(current_minutes);
    LOG_INFO("Initial mode: %s", warm_mode ? "WARM" : "COLD");
    if (warm_mode){
        sendCommand(Command::YELLOW);
    }
    else {
        sendCommand(Command::BLUE);
    }
    return true;
}

void Lights::checkAndUpdateMode(uint16_t current_minutes) {
    bool new_mode = determineMode(current_minutes);
    LOG_INFO("Current mode: %s", new_mode ? "WARM" : "COLD");
    if (new_mode != warm_mode) {
        warm_mode = new_mode;
        LOG_INFO("Mode changed to %s", warm_mode ? "WARM" : "COLD");
        warm_mode ? sendCommand(Command::YELLOW) : sendCommand(Command::BLUE);
    }
}

void Lights::adjustBrightness() {
    float current_lux = getLuxValue();
    LOG_INFO("Current lux: %.2f", current_lux);

    if (current_lux < DARK_THRESHOLD) {
        if (!max_brightness){
            min_brightness = false;
            LOG_INFO("Increasing brightness.");
            uint8_t failures = 0;
            while(getLuxValue() < DARK_THRESHOLD) {
                if (sendCommand(Command::MORE_LIGHT) != CommandResult::POSITIVE) {
                    failures++;
                    if (failures >= MAX_FAILURES) {
                        LOG_INFO("Max brightness reached.");
                        break;
                    }
                }
            }
        }
    }
    else if (current_lux > BRIGHT_THRESHOLD) {
        if (!min_brightness){
            max_brightness = false;
            LOG_INFO("Decreasing brightness.");
            uint8_t failures = 0;
            while(getLuxValue() > BRIGHT_THRESHOLD) {
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

bool Lights::isEnoughLight() {
    float current_lux = getLuxValue();
    LOG_INFO("Current lux for isEnoughLight(): %.2f", current_lux);
    if (current_lux >= DARK_THRESHOLD){
        return true;
    }
    return false;
}

float Lights::getLuxValue() {
    float lux = 0.0f;
    xSemaphoreTake(lightsSensorMutex, portMAX_DELAY);
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ch0 = lum & 0xFFFF;       // IR + Visible
    uint16_t ch1 = lum >> 16;          // IR
    lux = tsl.calculateLux(ch0, ch1);
    if (isnan(lux)) {
        // In some conditions, TSL library can return NaN if sensor saturates
        LOG_WARNING("TSL2591 returned NaN, forcing to 0.0");
        return 0.0f;
    }
    xSemaphoreGive(lightsSensorMutex);
    return lux;
}

bool Lights::initTSL2591() {
    if (!tsl.begin()) {
        LOG_ERROR("TSL2591 not found (I2C).");
        return false;
    }

    // Configure gain and integration time as needed
    tsl.setGain(TSL2591_GAIN_MED); // Adjust gain based on environment
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS); // Adjust integration time based on requirements

    LOG_INFO("TSL2591 initialized successfully");
    return true;
}

bool Lights::send(Command command) {
    bool success = false;
    for (uint8_t i = 0; i < MAX_TRANSMIT_RETRIES; i++) {
        switch(command) {
            case Command::ON:
            case Command::OFF:
                // ON and OFF share the same 433MHz signal
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
        if (success) {
            return true;
        }
    }
    return false;
}

void Lights::transmit(const char* bits) {
    for (size_t i = 0; i < strlen(bits); i++) {
        digitalWrite(TRANSMITTER_PIN, (bits[i] == '1') ? HIGH : LOW);
        delayMicroseconds(DIGIT_DURATION);
    }
    digitalWrite(TRANSMITTER_PIN, LOW);
}

bool Lights::sendSignal(const char* command, const char* repeat) {
    transmit(command);
    for (int i = 0; i < NUM_REPEATS; i++) {
        // Wait between repeats
        uint8_t time_ms = PAUSE_US / 1000;
        uint8_t remaining_time_us = PAUSE_US % 1000;
        vTaskDelay(pdMS_TO_TICKS(time_ms));
        delayMicroseconds(remaining_time_us);
        transmit(repeat);
    }

    // Release the mutex after transmission
    xSemaphoreGive(transmitterMutex);
    return true;
}
