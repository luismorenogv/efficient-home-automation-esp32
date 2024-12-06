/**
 * @file DataManager.cpp
 * @brief Implementation of DataManager class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 4, 2024
 */

#include "RoomNode/DataManager.h"

DataManager::DataManager(): presence(false) {
    presenceMutex = xSemaphoreCreateMutex();
    if (presenceMutex == NULL) {
        Serial.println("Failed to create presenceMutex semaphore!");
    } else {
        Serial.println("presenceMutex semaphore created successfully.");
    }
}

void DataManager::setPresence(bool new_presence){
    xSemaphoreTake(presenceMutex, portMAX_DELAY);
        presence = new_presence;
    xSemaphoreGive(presenceMutex);
}