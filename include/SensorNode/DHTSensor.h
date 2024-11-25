/**
 * @file DHTSensor.h
 * @brief Declaration of DHTSensor class for reading from the DHT sensor
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include <DHT.h>

constexpr uint8_t DHT_TYPE = DHT22;

class DHTSensor {
public:
    DHTSensor(uint8_t pin, uint8_t type);
    void initialize();
    bool readSensorData(float& temperature, float& humidity);

private:
    DHT dht;
};
