/**
 * @file SensorNode.h
 * @brief Declaration of SensorNode class for handling sensor readings and ESP-NOW communication
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include "SensorNode/DHTSensor.h"
#include "SensorNode/ESPNowHandler.h"
#include "SensorNode/PowerManager.h"

class SensorNode {
public:
    SensorNode(const uint8_t room_id, uint32_t* sleep_duration);
    bool initialize(uint8_t wifi_channel);
    void run();
    void goSleep();
    bool joinNetwork();

private:
    uint8_t room_id;
    DHTSensor dhtSensor;
    ESPNowHandler espNowHandler;
    PowerManager powerManager;

    void sendData();
    bool waitForAck();
};
