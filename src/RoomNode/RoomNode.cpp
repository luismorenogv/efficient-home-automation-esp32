/**
 * @file RoomNode.cpp
 * @brief Implementation of RoomNode class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 2, 2024
 */

#include "RoomNode/RoomNode.h"

RoomNode::RoomNode(): communications(), dataManager(){
    instance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
    presenceQueue = xQueueCreate(10, sizeof(uint8_t));
}

void RoomNode::initialize(){
    Serial.begin(115200);
    delay(1000);

    communications.initializeWifi();
    if (!communications.initializeESPNOW()){
        esp_deep_sleep_start();
    }
    communications.registerPeer((uint8_t*)master_mac_addr);

    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    


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

    result = xTaskCreatePinnedToCore(
        lightsControlTask,
        "Lights Control Task",
        2048,
        this,
        2,
        &lightsControlTaskHandle,
        1
    );

    if (result != pdPASS) {
        Serial.println("Failed to create Lights Control Task");
    }

    result = xTaskCreatePinnedToCore(
        presenceTask,
        "Presence Task",
        2048,
        this,
        2,
        &presenceTaskHandle,
        1
    );

    if (result != pdPASS) {
        Serial.println("Failed to create Presence Task");
    }
}

void RoomNode::espnowTask(void* pvParameter) {
    RoomNode* self = static_cast<RoomNode*>(pvParameter);
    IncomingMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            MessageType msg_type = static_cast<MessageType>(msg.data[0]);
            switch (msg_type) {
                case MessageType::TEMP_HUMID:
                    {
                    }
                    break;
                case MessageType::JOIN_NETWORK:
                    {
                    }
                    break;
                case MessageType::ACK:
                    {
                    }
                    break;
                default:
                    Serial.printf("Received unknown message type: %d\r\n", msg_type);
            }
        }
    }
}