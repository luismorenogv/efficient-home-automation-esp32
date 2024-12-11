/**
 * @file SHT31Sensor.h
 * @brief Header file for SHT31Sensor class
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */
#pragma once
#include <SPI.h>
#include <Adafruit_SHT31.h>

class SHT31Sensor {
public:
    // Constructs with I2C address and optional SDA/SCL pins
    SHT31Sensor(uint8_t address = 0x44, uint8_t sda_pin = 21, uint8_t scl_pin = 22);

    // Initializes the SHT31 sensor
    bool initialize();

    // Reads temperature and humidity, returns true if successful
    bool readSensorData(float& temperature, float& humidity);

private:
    Adafruit_SHT31 sht31;
    uint8_t address;
};