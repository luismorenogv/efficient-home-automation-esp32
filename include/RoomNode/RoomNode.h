/**
 * @file RoomNode.h
 * @brief Declaration of RoomNode class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 2, 2024
 */
#pragma once

#include <Arduino.h>
#include "Common/NTPClient.h"
#include "Common/Communications.h"
#include "DataManager.h"


class RoomNode {
public:
    RoomNode();
    void initialize();

private:
    NTPClient ntpClient;
    Communications communications;
    DataManager dataManager;
    static RoomNode* intance;

};