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

Communications::Communications(DataManager& dataManager) : dataManager(dataManager) {
    instance = this;
}

void Communications::initializeWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\r\nWi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Master MAC Address: ");
    Serial.println(WiFi.macAddress());

    uint8_t wifi_channel = WiFi.channel();
    Serial.print("Wi-Fi Channel: ");
    Serial.println(wifi_channel);
}

bool Communications::initializeESPNOW(const uint8_t *const sensor_mac_addrs[], uint8_t num_nodes) {
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

    esp_now_register_recv_cb(Communications::onDataRecvStatic);

    Serial.println("ESP-NOW initialized");
    return true;
}

void Communications::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    Serial.printf("onDataRecvStatic invoked. MAC: %02X:%02X:%02X:%02X:%02X:%02X, len: %d\r\n",
                  mac_addr[0], mac_addr[1], mac_addr[2],
                  mac_addr[3], mac_addr[4], mac_addr[5], len);
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

            if (dataQueue) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xQueueSendFromISR(dataQueue, &msg, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }

            sendAck(mac_addr, MessageType::TEMP_HUMID);
        } else {
            Serial.println("Received TEMP_HUMID message with incorrect length.");
        }
    }
    else if (msg_type == MessageType::JOIN_NETWORK) {
        if (len == sizeof(JoinNetworkMsg)) {
            JoinNetworkMsg msg;
            memcpy(&msg, data, sizeof(JoinNetworkMsg));

            uint8_t room_id = msg.id;
            uint32_t wake_interval = msg.wake_interval_ms;
            dataManager.setWakeInterval(room_id, wake_interval);
            dataManager.setMacAddr(room_id, mac_addr);

            esp_now_peer_info_t peerInfo;
            memset(&peerInfo, 0, sizeof(peerInfo));
            memcpy(peerInfo.peer_addr, mac_addr, MAC_ADDRESS_LENGTH);
            peerInfo.channel = 0; // Use the current Wi-Fi channel
            peerInfo.encrypt = false;

            if (esp_now_add_peer(&peerInfo) == ESP_OK) {
                Serial.printf("SensorNode %u added as peer successfully: %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
                              room_id,
                              mac_addr[0], mac_addr[1], 
                              mac_addr[2], mac_addr[3], 
                              mac_addr[4], mac_addr[5]);
            } else {
                Serial.printf("Failed to add SensorNode %u as peer: %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
                              room_id,
                              mac_addr[0], mac_addr[1], 
                              mac_addr[2], mac_addr[3], 
                              mac_addr[4], mac_addr[5]);
            }

            Serial.printf("Received JOIN_NETWORK from room %u with wake_interval %u ms\r\n", room_id, wake_interval);

            sendAck(mac_addr, MessageType::JOIN_NETWORK);
        } else {
            Serial.println("Received JOIN_NETWORK message with incorrect length.");
        }
    }
    else {
        Serial.println("Received unknown or unhandled message type.");
    }
}

void Communications::sendAck(const uint8_t* mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;

    esp_err_t result = esp_now_send(mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(AckMsg));
    if (result == ESP_OK) {
        Serial.printf("ACK sent to sensor %02X:%02X:%02X:%02X:%02X:%02X successfully\r\n",
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        Serial.printf("Error sending ACK: %d\r\n", result);
    }
}

void Communications::sendMsg(const uint8_t* mac_addr, const uint8_t* data, size_t size) {
    esp_err_t result = esp_now_send(mac_addr, data, size);
    if (result == ESP_OK) {
        Serial.printf("Message sent successfully to %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                      mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        Serial.printf("Error sending message: %d\r\n", result);
    }
}

void Communications::setQueue(QueueHandle_t queue) {
    dataQueue = queue;
}