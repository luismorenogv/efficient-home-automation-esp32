/**
 * @file CommunicationsBase.cpp
 * @brief Implementation of CommunicationsBase class for ESP-NOW and WiFi communication
 * 
 * Handles initialization, peer management, message sending, and receiving.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
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

// Initializes Wi-Fi in station mode and connects to the network
void CommunicationsBase::initializeWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    LOG_INFO("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        #ifdef ENABLE_LOGGING
            Serial.print(".");
        #endif
    }
    LOG_INFO("Wi-Fi connected");
    LOG_INFO("IP Address: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("MAC Address: %s", WiFi.macAddress().c_str());
    LOG_INFO("Wi-Fi Channel: %u", WiFi.channel());
}

// Initializes ESP-NOW and registers the receive callback
bool CommunicationsBase::initializeESPNOW() {
    const int MAX_INIT_RETRIES = 5;
    bool success = false;
    for (int attempt = 0; attempt < MAX_INIT_RETRIES; ++attempt) {
        if (esp_now_init() == ESP_OK) {
            success = true;
            break;
        }
        LOG_INFO("Retrying ESP-NOW initialization...");
        delay(1000);
    }

    if (!success) {
        LOG_ERROR("Failed to initialize ESP-NOW after multiple attempts.");
        return false;
    }

    // Register receive callback
    esp_now_register_recv_cb(CommunicationsBase::onDataRecvStatic);

    LOG_INFO("ESP-NOW initialized");
    return true;
}

// Registers a new peer device
bool CommunicationsBase::registerPeer(uint8_t* mac_address, uint8_t wifi_channel) {
    esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE); // Set WiFi channel of device accordingly
    
    // Lock the mutex to protect peer list
    if (xSemaphoreTake(peerMutex, portMAX_DELAY) != pdTRUE) {
        LOG_WARNING("Failed to take peer mutex.");
        return false;
    }

    if (numPeers >= MAX_PEERS) {
        LOG_INFO("Maximum number of peers reached.");
        xSemaphoreGive(peerMutex);
        return false;
    }

    // Check if peer already exists
    for (int i = 0; i < numPeers; ++i) {
        if (memcmp(peers[i].mac_addr, mac_address, MAC_ADDRESS_LENGTH) == 0) {
            LOG_INFO("Peer already registered.");
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
        LOG_ERROR("Failed to add peer");
        xSemaphoreGive(peerMutex);
        return false;
    }

    numPeers++;

    LOG_INFO("Peer registered: ");
    char mac_str[MAC_ADDRESS_LENGTH * 3] = {0};
    for (int i = 0; i < MAC_ADDRESS_LENGTH; i++) {
        snprintf(mac_str + i * 3, 4, "%02X%s", mac_address[i], (i < MAC_ADDRESS_LENGTH - 1) ? ":" : "");
    }
    LOG_INFO("MAC Address: %s", mac_str);

    xSemaphoreGive(peerMutex);
    return true;
}

// Unregisters an existing peer device
bool CommunicationsBase::unregisterPeer(uint8_t* mac_address) {
    // Lock the mutex to protect the peer list
    if (xSemaphoreTake(peerMutex, portMAX_DELAY) != pdTRUE) {
        LOG_ERROR("Failed to take peer mutex.");
        return false;
    }

    // Find the peer in the list
    for (int i = 0; i < numPeers; ++i) {
        if (memcmp(peers[i].mac_addr, mac_address, MAC_ADDRESS_LENGTH) == 0) {
            // Remove the peer from ESP-NOW
            if (esp_now_del_peer(mac_address) != ESP_OK) {
                LOG_ERROR("Failed to remove peer from ESP-NOW.");
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
            LOG_INFO("Peer unregistered successfully.");

            xSemaphoreGive(peerMutex);
            return true;
        }
    }

    LOG_WARNING("Peer not found.");
    xSemaphoreGive(peerMutex);
    return false;
}

// Sends a message to a specified peer
bool CommunicationsBase::sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size) {
    esp_err_t result = esp_now_send(mac_addr, data, size);
    if (result == ESP_OK) {
        LOG_INFO("%s message sent successfully to %02X:%02X:%02X:%02X:%02X:%02X\r\n", MSG_NAME[data[0]],
                      mac_addr[0], mac_addr[1], mac_addr[2],
                      mac_addr[3], mac_addr[4], mac_addr[5]);
        return true;
    } else {
        LOG_ERROR("Error sending message: %d", result);
        return false;
    }
}

// Sends an acknowledgment message to a specified peer
void CommunicationsBase::sendAck(const uint8_t* mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;
    sendMsg((uint8_t*)mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(ack));
}

// Sets the message queue for incoming messages
void CommunicationsBase::setQueue(QueueHandle_t queue) {
    dataQueue = queue;
}

// Static callback to handle incoming ESP-NOW data
void CommunicationsBase::onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (instance) {
        instance->onDataRecv(mac_addr, data, len);
    }
}

// Handles received ESP-NOW data by enqueuing the message
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
