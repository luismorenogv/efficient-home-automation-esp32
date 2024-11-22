/**
 * @file Communications.cpp
 * @brief Implementation of Communications class for ESP-NOW communication with SensorNodes
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#include "MasterDevice/Communications.h"
#include "secrets.h" // For Wi-Fi credentials

Communications* Communications::instance = nullptr;

Communications::Communications(){
    instance = this;
}

void Communications::initializeWifi() {
    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false); // Disable Wi-Fi sleep

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Get Wi-Fi channel
    uint8_t wifi_channel = WiFi.channel();
    Serial.print("Wi-Fi Channel: ");
    Serial.println(wifi_channel);

}
bool Communications::initializeESPNOW(const uint8_t *const sensor_mac_addrs[], uint8_t num_nodes) {
    // Initialize ESP-NOW
    const int MAX_INIT_RETRIES = 5;
    bool success = false;
    for (int attempt = 0; attempt < MAX_INIT_RETRIES; ++attempt) {
        if (esp_now_init() == ESP_OK) {
            success = true;
            break;
        }
        Serial.println("Retrying ESP-NOW initialization...");
        delay(1000);
    }

    if (!success) {
        Serial.println("Failed to initialize ESP-NOW after multiple attempts.");
        return false;
    }

    // Add each SensorNode as a peer
    for (uint8_t i = 0; i < num_nodes; ++i) {
        esp_now_peer_info_t sensorPeer;
        memset(&sensorPeer, 0, sizeof(sensorPeer));
        memcpy(sensorPeer.peer_addr, sensor_mac_addrs[i], MAC_ADDRESS_LENGTH);
        sensorPeer.channel = 0; // Use current Wi-Fi channel
        sensorPeer.encrypt = false;

        if (esp_now_add_peer(&sensorPeer) == ESP_OK) {
            Serial.printf("SensorNode %u added as peer successfully\n", i + 1);
        } else {
            Serial.printf("Failed to add SensorNode %u as peer\n", i + 1);
            // Optionally handle individual peer addition failures
        }
    }

    // Register receive callback
    esp_now_register_recv_cb(Communications::onDataRecvStatic);

    Serial.println("ESPNOW initialized");
    return true;
}

void Communications::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (instance) {
        instance->onDataRecv(mac_addr, data, len);
    }
}

void Communications::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);

    if (msg_type == MessageType::TEMP_HUMID) {
        if (len == sizeof(TempHumidMsg)) {
            TempHumidMsg msg;
            memcpy(&msg, data, sizeof(TempHumidMsg));

            // Enqueue the message
            if (dataQueue) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                if (xQueueSendFromISR(dataQueue, &msg, &xHigherPriorityTaskWoken) != pdTRUE) {
                    Serial.println("Failed to enqueue ESP-NOW message");
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }

            // Send ACK
            sendAck(mac_addr, MessageType::TEMP_HUMID);
        } else {
            Serial.println("Received TEMP_HUMID message with incorrect length.");
        }
    } else {
        Serial.println("Received unknown or unhandled message type.");
    }
}

void Communications::sendAck(const uint8_t* mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;

    esp_err_t result = esp_now_send(mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(AckMsg));
    if (result == ESP_OK) {
        Serial.printf("ACK sent to sensor %02X:%02X:%02X:%02X:%02X:%02X successfully\n",
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        Serial.printf("Error sending ACK: %d\n", result);
    }
}

void Communications::setQueue(QueueHandle_t queue) {
    dataQueue = queue;
}
