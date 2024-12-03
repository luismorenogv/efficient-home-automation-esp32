/**
 * @file SensorNode.h
 * @brief Declaration of SensorNode class for handling sensor readings and ESP-NOW communication
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include "SHT31Sensor.h"
#include "ESPNowHandler.h"
#include "PowerManager.h"

constexpr uint8_t SHT31_ADDRESS = 0x44;
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;

class SensorNode {
public:
    SensorNode(const uint8_t room_id, uint32_t* sleep_duration);
    bool initialize(uint8_t wifi_channel);
    void run();
    void goSleep();
    bool joinNetwork();

private:
    uint8_t room_id;
    SHT31Sensor sht31Sensor;
    ESPNowHandler espNowHandler;
    PowerManager powerManager;

    void sendData();
    bool waitForAck();
};
