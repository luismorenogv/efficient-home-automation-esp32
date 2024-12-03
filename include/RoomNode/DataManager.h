/**
 * @file DataManager.h
 * @brief Declaration of DataManager class for managing room data storage and retrieval
 *
 * @author Luis Moreno
 * @date Dec 3, 2024
 */

#pragma once

#include <Arduino.h>
#include <freertos/semphr.h>

class DataManager {
public:
    DataManager();
    // Delete copy constructor and copy assignment operator
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
private:
};
