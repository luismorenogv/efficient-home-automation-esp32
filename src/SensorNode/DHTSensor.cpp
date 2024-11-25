/**
 * @file DHTSensor.cpp
 * @brief Implementation of DHTSensor class for reading from the DHT sensor
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "SensorNode/DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : dht(pin, type) {
}

void DHTSensor::initialize() {
    dht.begin();
}

bool DHTSensor::readSensorData(float& temperature, float& humidity) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        return false; // Reading failed
    }
    return true; // Reading successful
}
