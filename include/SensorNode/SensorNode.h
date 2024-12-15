/**
 * @file SensorNode.h
 * @brief Manages sensor readings and communication for the SensorNode device.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */
#pragma once

#include "SHT31Sensor.h"
#include "ESPNowHandler.h"
#include "PowerManager.h"
#include "esp_wifi.h"

constexpr uint8_t SHT31_ADDRESS = 0x44;
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;

class SensorNode {
public:
    // Constructs with room ID, pointers to stored settings, and first_cycle flag
    SensorNode(uint8_t room_id, uint32_t* sleep_duration, uint8_t* channel_wifi, bool* first_cycle);

    // Initializes sensor and ESP-NOW communication
    bool initialize();

    // Reads sensor data and sends it, if joined to the master
    void run();

    // Enters deep sleep permanently or with wake up by timer configured
    void goSleep(bool permanent);

    // Attempts to join master network on first run
    bool joinNetwork();

private:
    uint8_t room_id;
    SHT31Sensor sht31Sensor;
    ESPNowHandler espNowHandler;
    PowerManager powerManager;
    uint8_t* channel_wifi;
    bool* first_cycle;

    // Sends sensor data message
    void sendData();

    // Waits for an ACK after sending a message
    bool waitForAck();
};