/**
 * @file SensorNode.h
 * @brief Manages sensor readings and communication for the SensorNode device.
 * 
 * On the first cycle, attempts to join the network and synchronize with MasterDevice.
 * On subsequent cycles, sends temperature/humidity data, and then sleeps.
 * 
 * Uses SHT31 for more accurate readings compared to DHT22.
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
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
    SensorNode(const uint8_t room_id, uint32_t* sleep_duration, uint8_t* channel_wifi, bool* first_cycle);
    bool initialize();
    void run();
    void goSleep();
    bool joinNetwork();

private:
    uint8_t room_id;
    SHT31Sensor sht31Sensor;
    ESPNowHandler espNowHandler;
    PowerManager powerManager;

    uint8_t* channel_wifi;
    bool* first_cycle;

    void sendData();
    bool waitForAck();
};
