/**
 * @file AirConditioner.h
 * @brief Controls a Panasonic AC via IR signals for the RoomNode.
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <Arduino.h>
#include "config.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Panasonic.h>

class AirConditioner {
public:
    AirConditioner();

    // Prevent multiple instances
    AirConditioner(const AirConditioner&) = delete;
    AirConditioner& operator=(const AirConditioner&) = delete;

    // Sends IR command to turn the AC off
    void turnOff();

private:
    IRPanasonicAc ac;
};
