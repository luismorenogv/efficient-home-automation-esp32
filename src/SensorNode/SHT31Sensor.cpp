/**
 * @file SHT31Sensor.cpp
 * @brief Implementation of SHT31Sensor class for reading from the SHT31 sensor
 * 
 * Provides more accurate and stable readings compared to DHT sensors.
 * 
 * @date Dec, 2024
 */

#include "SensorNode/SHT31Sensor.h"
#include <Wire.h>

SHT31Sensor::SHT31Sensor(uint8_t address, uint8_t sda_pin, uint8_t scl_pin) : sht31() {
    this->address = address;
    Wire.begin(sda_pin, scl_pin);
}

bool SHT31Sensor::initialize() {
    Wire.begin();
    if (!sht31.begin(this->address)) {
        Serial.println("Couldn't find SHT31 sensor");
        return false;
    }
    Serial.println("SHT31 initialized");
    return true;
}

bool SHT31Sensor::readSensorData(float& temperature, float& humidity) {
    temperature = sht31.readTemperature();
    humidity = sht31.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Invalid readings from SHT31 sensor");
        return false;
    }
    return true;
}
