/**
 * @file AirConditioner.h
 * @brief Declaration of AirConditioner class for controlling the Panasonic AC with IR signals
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
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

    // Delete copy constructor and assignment operator to prevent multiple instances
    AirConditioner(const AirConditioner&) = delete;
    AirConditioner& operator=(const AirConditioner&) = delete;

    void turnOff();

private:
    IRPanasonicAc ac;
};
