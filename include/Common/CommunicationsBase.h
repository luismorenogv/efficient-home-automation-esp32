/**
 * @file CommunicationsBase.h
 * @brief Abstract base class for ESP-NOW and WiFi communication
 * 
 * Derived classes implement device-specific message handling.
 * Uses a static callback to bridge ESP-NOW events to instance methods.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include "config.h"
#include "common.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Structure to represent a peer device
struct Peer {
    uint8_t mac_addr[MAC_ADDRESS_LENGTH];
    esp_now_peer_info_t peer_info;
};

// Base class for ESP-NOW and WiFi communication
class CommunicationsBase {
public:
    CommunicationsBase();
    virtual ~CommunicationsBase();

    // Initializes Wi-Fi connection
    void initializeWifi();

    // Initializes ESP-NOW communication
    bool initializeESPNOW();

    // Registers a peer with given MAC address and Wi-Fi channel
    bool registerPeer(uint8_t* mac_address, uint8_t wifi_channel);

    // Unregisters a peer with given MAC address
    bool unregisterPeer(uint8_t* mac_address);

    // Sends a message to a peer
    bool sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size);

    // Sends an acknowledgment message to a peer
    void sendAck(const uint8_t* mac_addr, MessageType acked_msg);

    // Sets the message queue
    void setQueue(QueueHandle_t queue);

protected:
    // Static callback for receiving data via ESP-NOW
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len);

    // Pure virtual method to handle received data
    virtual void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) = 0;

    static CommunicationsBase* instance; // Singleton instance

    QueueHandle_t dataQueue; // Queue for incoming messages

    Peer peers[MAX_PEERS]; // Array of registered peers
    int numPeers;          // Number of registered peers

    SemaphoreHandle_t peerMutex; // Mutex to protect peer list
};
