/**
 * @file CommunicationsBase.h
 * @brief Abstract base class for ESP-NOW and WiFi communication
 * 
 * Provides common functionalities for MasterDevice, SensorNode, and RoomNode.
 * Derived classes should implement device-specific message handling.
 * 
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

#include <esp_now.h>
#include <WiFi.h>
#include "config.h"
#include "common.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>


class CommunicationsBase {
public:
    CommunicationsBase();
    virtual ~CommunicationsBase();

    // Initialize Wi-Fi
    void initializeWifi();

    // Initialize ESP-NOW
    bool initializeESPNOW();

    // Register a new peer
    bool registerPeer(uint8_t* mac_address, uint8_t wifi_channel);

    // Unregister a peer
    bool unregisterPeer(uint8_t* mac_address);

    // Send a message to a specific peer
    bool sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size);

    // Send an ACK to a peer
    void sendAck(const uint8_t* mac_addr, MessageType acked_msg);

    // Set the queue for incoming messages
    void setQueue(QueueHandle_t queue);

protected:
    // Static callback for ESP-NOW reception
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len);

    // Instance-specific callback
    virtual void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) = 0;

    // Instance pointer for static callback
    static CommunicationsBase* instance;

    // Queue for incoming messages
    QueueHandle_t dataQueue;

    // Array of peers
    struct Peer {
        uint8_t mac_addr[MAC_ADDRESS_LENGTH];
        esp_now_peer_info_t peer_info;
    };
    Peer peers[MAX_PEERS];
    int numPeers;

    // Mutex to protect peer list
    SemaphoreHandle_t peerMutex;
};
