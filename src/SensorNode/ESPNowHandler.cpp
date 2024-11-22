/**
 * @file ESPNowHandler.cpp
 * @brief Implementation of ESPNowHandler class for ESP-NOW communication with MasterDevice
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include "SensorNode/ESPNowHandler.h"

ESPNowHandler* ESPNowHandler::instance = nullptr;

ESPNowHandler::ESPNowHandler() : ack_received(false), last_acked_msg(MessageType::ACK) {
    instance = this;
}

bool ESPNowHandler::initializeESPNOW(const uint8_t* master_mac_address) {
    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize ESP-NOW
    bool success = false;
    for (int attempt = 0; attempt < MAX_INIT_RETRIES; ++attempt) {
        if (esp_now_init() == ESP_OK) {
            success = true;
            break;
        }
        Serial.println("Retrying ESP-NOW initialization...");
        delay(1500);
    }

    if (!success) {
        Serial.println("Failed to initialize ESP-NOW after multiple attempts.");
        return false;
    }

    // Register receive callback
    esp_now_register_recv_cb(ESPNowHandler::onDataRecvStatic);

    // Register master as peer
    memset(&master_peer, 0, sizeof(master_peer));
    memcpy(master_peer.peer_addr, master_mac_address, MAC_ADDRESS_LENGTH);
    master_peer.channel = 0; // Use current Wi-Fi channel
    master_peer.encrypt = false;

    if (esp_now_add_peer(&master_peer) != ESP_OK){
        Serial.println("Failed to add master as peer");
        return false;
    }

    Serial.println("ESPNOW initialized");

    return true;
}

void ESPNowHandler::sendData(const TempHumidMsg& data) {
    esp_err_t result = esp_now_send(master_peer.peer_addr, reinterpret_cast<const uint8_t*>(&data), sizeof(TempHumidMsg));
    if (result == ESP_OK) {
        Serial.println("Data sent successfully");
    } else {
        Serial.printf("Error sending data: %d\n", result);
    }
}

bool ESPNowHandler::waitForAck(MessageType expected_ack, unsigned long timeout_ms) {
    ack_received = false;
    last_acked_msg = expected_ack;

    unsigned long start_time = millis();
    while ((millis() - start_time) < timeout_ms) {
        if (ack_received) {
            return true;
        }
        delay(10); // Small delay to yield control
    }
    return false;
}

void ESPNowHandler::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (instance) {
        instance->onDataRecv(mac_addr, data, len);
    }
}

void ESPNowHandler::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);
    if (msg_type == MessageType::ACK) {
        if (len >= sizeof(AckMsg)) {
            const AckMsg* ack = reinterpret_cast<const AckMsg*>(data);
            if (ack->acked_msg == last_acked_msg) {
                ack_received = true;
                Serial.println("ACK received from master");
            }
        } else {
            Serial.println("ACK received with incorrect length.");
        }
    } else {
        Serial.println("Received unknown or unhandled message type.");
    }
}
