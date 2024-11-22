/**
 * @file DHTSensor.h
 * @brief Declaration of DHTSensor class for reading from the DHT sensor
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#pragma once

#include <DHT.h>



class DHTSensor {
public:
    DHTSensor(uint8_t pin, uint8_t type);
    void initialize();
    bool readSensorData(float& temperature, float& humidity);

private:
    DHT dht;
};
