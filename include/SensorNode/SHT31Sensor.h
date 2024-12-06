/**
 * @file SHT31Sensor.h
 * @brief Header file for SHT31Sensor class
 * 
 * Provides reliable temperature and humidity readings.
 * 
 * SDA and SCL pins can be passed to the constructor to match the board configuration.
 * 
 * @author Luis Moreno
 * @date Dec 3, 2024
 */
#pragma once
#include <SPI.h>
#include <Adafruit_SHT31.h>

class SHT31Sensor {
public:
    SHT31Sensor(uint8_t address = 0x44, uint8_t sda_pin = 21, uint8_t scl_pin = 22);
    bool initialize();
    bool readSensorData(float& temperature, float& humidity);

private:
    Adafruit_SHT31 sht31;
    uint8_t address;
};
