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

MasterController::MasterController() : webSockets(dataManager) {
    espnowQueue = xQueueCreate(10, sizeof(TempHumidMsg));
}

void MasterController::initialize() {
    Serial.begin(115200);
    delay(1000);

    // Initialize modules
    communications.initializeWifi();
    if (!communications.initializeESPNOW(NODE_MAC_ADDRS, AVAILABLE_NODES)){
        // TODO: Send telegram alert
        esp_deep_sleep_start();
    };
    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    webServer.initialize();

    // Create tasks
    BaseType_t result;

    // ESP-NOW Task: High priority, handles ESP-NOW communication
    result = xTaskCreatePinnedToCore(
        espnowTask,
        "ESP-NOW Task",
        4096,
        this,
        2, // Higher priority
        &espnowTaskHandle,
        1  // Pin to core 1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create ESP-NOW Task");
    }

    // Web Server Task: Lower priority, handles web server and WebSocket communication
    result = xTaskCreatePinnedToCore(
        webServerTask,
        "Web Server Task",
        8192,
        this,
        1, // Lower priority
        &webServerTaskHandle,
        0  // Pin to core 0
    );
    if (result != pdPASS) {
        Serial.println("Failed to create Web Server Task");
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
        }
    }
}

void MasterController::webServerTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);

    // Initialize WebSocketServer
    self->webSockets.initialize(self->webServer.getServer());

    // Start web server
    self->webServer.start();

    // Run the server (could include additional server maintenance tasks)
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust as needed
    }
}
