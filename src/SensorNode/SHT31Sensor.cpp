/**
 * @file SHT31Sensor.cpp
 * @brief Implementation of SHT31Sensor class for reading from the SHT31 sensor
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "SensorNode/SHT31Sensor.h"
#include <Wire.h>

SHT31Sensor::SHT31Sensor(uint8_t address, uint8_t sda_pin, uint8_t scl_pin) : sht31(), address(address) {
    Wire.begin(sda_pin, scl_pin);
}

bool SHT31Sensor::initialize() {
    Wire.begin();
    if (!sht31.begin(this->address)) {
        Serial.println("SHT31 not found");
        return false;
    }
    Serial.println("SHT31 ready");
    return true;
}

bool SHT31Sensor::readSensorData(float& temperature, float& humidity) {
    temperature = sht31.readTemperature();
    humidity = sht31.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Invalid SHT31 readings");
        return false;
    }
    return true;
}