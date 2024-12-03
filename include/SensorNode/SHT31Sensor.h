/**
 * @file SHT31Sensor.h
 * @brief Header file for SHT31Sensor class
 *
 * @author Luis Moreno
 * @date Dec 3, 2024
 */

#pragma
#include <SPI.h>
#include <Adafruit_SHT31.h>

class SHT31Sensor {
public:
    /**
     * @brief Constructor for SHT31Sensor
     * 
     * @param address I2C address of the SHT31 sensor (default is 0x44)
     */
    SHT31Sensor(uint8_t address = 0x44, uint8_t sda_pin = 21, uint8_t scl_pin = 22);

    /**
     * @brief Initializes the SHT31 sensor
     * 
     * @return true if initialization is successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Reads temperature and humidity data from the sensor
     * 
     * @param temperature Reference to store the temperature value
     * @param humidity Reference to store the humidity value
     * @return true if data is read successfully, false otherwise
     */
    bool readSensorData(float& temperature, float& humidity);

private:
    Adafruit_SHT31 sht31;
    uint8_t address;
};