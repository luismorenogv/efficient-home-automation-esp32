/**
 * @file CommunicationsBase.cpp
 * @brief Implementation of CommunicationsBase class for ESP-NOW and WiFi communication
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#include "Common/CommunicationsBase.h"
#include "Common/secrets.h" // For Wi-Fi credentials
#include <string.h>

CommunicationsBase* CommunicationsBase::instance = nullptr;

CommunicationsBase::CommunicationsBase() : numPeers(0) {
    instance = this;
    peerMutex = xSemaphoreCreateMutex();

    // Initialize peers array
    for (int i = 0; i < MAX_PEERS; ++i) {
        memset(peers[i].mac_addr, 0, MAC_ADDRESS_LENGTH);
        memset(&peers[i].peer_info, 0, sizeof(esp_now_peer_info_t));
    }
}

CommunicationsBase::~CommunicationsBase() {
    // Clean up mutex
    if (peerMutex != nullptr) {
        vSemaphoreDelete(peerMutex);
        peerMutex = nullptr;
    }

    instance = nullptr;
}

void CommunicationsBase::initializeWifi() {
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

    Serial.print("Device MAC Address: ");
    Serial.println(WiFi.macAddress());

    uint8_t wifi_channel = WiFi.channel();
    Serial.print("Wi-Fi Channel: ");
    Serial.println(wifi_channel);
}

bool CommunicationsBase::initializeESPNOW() {
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

    // Register receive callback
    esp_now_register_recv_cb(CommunicationsBase::onDataRecvStatic);

    Serial.println("ESP-NOW initialized");
    return true;
}

bool CommunicationsBase::registerPeer(uint8_t* mac_address, uint8_t wifi_channel) {
    // Lock the mutex to protect peer list
    if (xSemaphoreTake(peerMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Failed to take peer mutex.");
        return false;
    }

    if (numPeers >= MAX_PEERS) {
        Serial.println("Maximum number of peers reached.");
        xSemaphoreGive(peerMutex);
        return false;
    }

    // Check if peer already exists
    for (int i = 0; i < numPeers; ++i) {
        if (memcmp(peers[i].mac_addr, mac_address, MAC_ADDRESS_LENGTH) == 0) {
            Serial.println("Peer already registered.");
            xSemaphoreGive(peerMutex);
            return false;
        }
    }

    // Add the new peer
    memcpy(peers[numPeers].mac_addr, mac_address, MAC_ADDRESS_LENGTH);
    memcpy(peers[numPeers].peer_info.peer_addr, mac_address, MAC_ADDRESS_LENGTH);
    peers[numPeers].peer_info.channel = wifi_channel;
    peers[numPeers].peer_info.encrypt = false;

    if (esp_now_add_peer(&peers[numPeers].peer_info) != ESP_OK) {
        Serial.println("Failed to add peer");
        xSemaphoreGive(peerMutex);
        return false;
    }

    numPeers++;

    Serial.print("Peer registered: ");
    for(int i =0; i<MAC_ADDRESS_LENGTH; i++) {
        if(mac_address[i] < 16) Serial.print("0");
        Serial.print(mac_address[i], HEX);
        if(i < MAC_ADDRESS_LENGTH - 1) Serial.print(":");
    }
    Serial.println();

    xSemaphoreGive(peerMutex);
    return true;
}

bool CommunicationsBase::unregisterPeer(uint8_t* mac_address) {
    // Lock the mutex to protect the peer list
    if (xSemaphoreTake(peerMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("Failed to take peer mutex.");
        return false;
    }

    // Find the peer in the list
    for (int i = 0; i < numPeers; ++i) {
        if (memcmp(peers[i].mac_addr, mac_address, MAC_ADDRESS_LENGTH) == 0) {
            // Remove the peer from ESP-NOW
            if (esp_now_del_peer(mac_address) != ESP_OK) {
                Serial.println("Failed to remove peer from ESP-NOW.");
                xSemaphoreGive(peerMutex);
                return false;
            }

            // Shift remaining peers to fill the gap
            for (int j = i; j < numPeers - 1; ++j) {
                peers[j] = peers[j + 1];
            }

            // Clear the last peer entry
            memset(&peers[numPeers - 1], 0, sizeof(Peer));

            numPeers--;
            Serial.println("Peer unregistered successfully.");

            xSemaphoreGive(peerMutex);
            return true;
        }
    }

    Serial.println("Peer not found.");
    xSemaphoreGive(peerMutex);
    return false;
}


bool CommunicationsBase::sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size) {
    esp_err_t result = esp_now_send(mac_addr, data, size);
    if (result == ESP_OK) {
        Serial.printf("Message sent successfully to %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                      mac_addr[0], mac_addr[1], mac_addr[2],
                      mac_addr[3], mac_addr[4], mac_addr[5]);
        return true;
    } else {
        Serial.printf("Error sending message: %d\r\n", result);
        return false;
    }
}

void CommunicationsBase::sendAck(const uint8_t* mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;
    sendMsg((uint8_t*)mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(ack));
}

void CommunicationsBase::setQueue(QueueHandle_t queue) {
    dataQueue = queue;
}

void CommunicationsBase::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (instance) {
        instance->onDataRecv(mac_addr, data, len);
    }
}

void CommunicationsBase::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len){
    // Enqueue the message for processing in the FreeRTOS task
    IncomingMsg incoming_msg;
    memcpy(incoming_msg.mac_addr, mac_addr, MAC_ADDRESS_LENGTH);
    memcpy(incoming_msg.data, data, len);
    incoming_msg.len = len;

    if (dataQueue) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(dataQueue, &incoming_msg, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}