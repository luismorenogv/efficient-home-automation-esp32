/**
 * @file SHT31Sensor.cpp
 * @brief Implementation of SHT31Sensor class for reading from the SHT31 sensor
 *
 * Replaces DHTSensor with SHT31 for more accurate measurements.
 * 
 * @date Dec, 2024
 */

#include "SensorNode/SHT31Sensor.h"
#include <Wire.h>

// Constructor
SHT31Sensor::SHT31Sensor(uint8_t address, uint8_t sda_pin, uint8_t scl_pin) : sht31() { // Default constructor
    this->address = address; // Store the address for later use
    Wire.begin(sda_pin, scl_pin);
}

// Initialize the sensor
bool SHT31Sensor::initialize() {
    Wire.begin(); // Initialize I2C communication
    if (!sht31.begin(this->address)) { // Pass the address to begin()
        Serial.println("Couldn't find SHT31");
        return false;
    }
    Serial.println("SHT31 initialized");
    return true;
}

// Read sensor data
bool SHT31Sensor::readSensorData(float& temperature, float& humidity) {
    temperature = sht31.readTemperature();
    humidity = sht31.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from SHT31 sensor!");
        return false; // Reading failed
    }
    return true; // Reading successful
}
