/**
 * @file AirConditioner.cpp
 * @brief Implementation of AirConditioner class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/AirConditioner.h"

AirConditioner::AirConditioner(): ac(IR_LED_PIN){
    ac.begin();
    ac.setModel(kPanasonicUnknown);
}

void AirConditioner::turnOff(){
    ac.off();
    ac.send();
}