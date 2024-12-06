/**
 * @file RoomNode.cpp
 * @brief Implementation of RoomNode class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Dec 2, 2024
 */

#include "RoomNode/RoomNode.h"

RoomNode::RoomNode(uint8_t room_id): room_id(room_id), dataManager(), wifi_channel(0){
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

    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    WiFi.disconnect();

    //TODO: pause multitasking until run function

    BaseType_t result;

    // ESP-NOW Task
    result = xTaskCreate(
        espnowTask,
        "ESP-NOW Task",
        4096,
        this,
        2,
        &espnowTaskHandle
    );

    if (result != pdPASS) {
        Serial.println("Failed to create ESP-NOW Task");
    }

    result = xTaskCreate(
        lightsControlTask,
        "Lights Control Task",
        2048,
        this,
        2,
        &lightsControlTaskHandle
    );

    if (result != pdPASS) {
        Serial.println("Failed to create Lights Control Task");
    }

    result = xTaskCreate(
        presenceTask,
        "Presence Task",
        2048,
        this,
        2,
        &presenceTaskHandle
    );

    if (result != pdPASS) {
        Serial.println("Failed to create Presence Task");
    }

    result = xTaskCreate(
        NTPSyncTask,
        "NTPSync Task",
        128,
        this,
        2,
        &NTPSyncTaskHandle
    );

    if (result != pdPASS) {
        Serial.println("Failed to create NTPSync Task");
    }
}
bool RoomNode::joinNetwork(){
    JoinNetworkMsg msg;
    msg.room_id = room_id;
    msg.node_type = NodeType::ROOM;
    msg.sleep_period_ms = 0;

    for (uint8_t i = 0; i < MAX_WIFI_CHANNEL; i++){
        wifi_channel = (wifi_channel + i) % MAX_WIFI_CHANNEL;
        esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
        communications.registerPeer((uint8_t*)master_mac_addr, wifi_channel);
        // Send data with retries
        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            communications.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (communications.waitForAck((uint8_t*)master_mac_addr, MessageType::JOIN_NETWORK, ACK_TIMEOUT_MS)) {
                ack_received = true;
            } else {
                retries++;
                Serial.printf("No ACK received, retrying (%u/%u)\r\n", retries, MAX_RETRIES);
            }
        }

        if (ack_received) {
            Serial.printf("Master found in channel %u", wifi_channel);
            return true;
        }
        Serial.printf("Failed to receive ACK after maximum retries in channel %u\r\n", wifi_channel);
        communications.unregisterPeer((uint8_t*)master_mac_addr);
    }
    return false;
}

void RoomNode::run(){
    // TODO: Start multitask
    
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