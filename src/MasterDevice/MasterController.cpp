/**
 * @file MasterController.cpp
 * @brief Implementation of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include "MasterDevice/MasterController.h"
#include <Arduino.h>
#include "mac_addrs.h"
#include "MasterConfig.h"

// Global pointer to MasterController instance for static callbacks
static MasterController* masterControllerInstance = nullptr;

MasterController::MasterController() 
    : communications(dataManager), 
      webSockets(dataManager) 
{
    masterControllerInstance = this;
    espnowQueue = xQueueCreate(10, sizeof(TempHumidMsg));
}

void MasterController::initialize() {
    Serial.begin(115200);
    delay(1000);

    communications.initializeWifi();
    if (!communications.initializeESPNOW(NODE_MAC_ADDRS, AVAILABLE_NODES)){
        esp_deep_sleep_start();
    }
    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    webServer.initialize();

    webSockets.initialize(webServer.getServer());
    webSockets.setPollingPeriodCallback(MasterController::pollingPeriodChangedCallback);

    BaseType_t result;

    // ESP-NOW Task
    result = xTaskCreatePinnedToCore(
        espnowTask,
        "ESP-NOW Task",
        4096,
        this,
        2,
        &espnowTaskHandle,
        1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create ESP-NOW Task");
    }

    // Web Server Task
    result = xTaskCreatePinnedToCore(
        webServerTask,
        "Web Server Task",
        8192,
        this,
        1,
        &webServerTaskHandle,
        0
    );
    if (result != pdPASS) {
        Serial.println("Failed to create Web Server Task");
    }
}

void MasterController::pollingPeriodChangedCallback(uint8_t room_id, uint32_t new_period_ms) {
    if (masterControllerInstance) {
        Serial.printf("Polling period changed for room %u: %u ms\n", room_id, new_period_ms);
        masterControllerInstance->dataManager.setPollingPeriodUpdate(room_id, new_period_ms);
    }
}

void MasterController::espnowTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    TempHumidMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            uint8_t room_id = msg.room_id;
            time_t timestamp = time(nullptr);

            self->dataManager.addData(room_id, msg.temperature, msg.humidity, timestamp);

            // Send update via WebSockets
            self->webSockets.sendDataUpdate(room_id);

            // Check if polling period update is pending
            if (self->dataManager.isPollingPeriodUpdated(room_id)) {
                uint32_t new_period = self->dataManager.getNewPollingPeriod(room_id);

                NewPollingPeriodMsg new_period_msg;
                new_period_msg.type = MessageType::NEW_POLLING_PERIOD;
                new_period_msg.new_period_ms = new_period;

                const uint8_t* sensor_mac = self->dataManager.getMacAddr(room_id);
                if (sensor_mac) {
                    self->communications.sendMsg(sensor_mac, reinterpret_cast<uint8_t*>(&new_period_msg), sizeof(NewPollingPeriodMsg));
                    Serial.printf("Sent NEW_POLLING_PERIOD to sensor in room %u successfully\n", room_id);
                }

                // Update wake_interval_ms and reset the flag
                self->dataManager.setWakeInterval(room_id, new_period);
                self->dataManager.setPollingPeriodUpdate(room_id, 0); // Reset update flag
            }
        }
    }
}

void MasterController::webServerTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);

    self->webServer.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
