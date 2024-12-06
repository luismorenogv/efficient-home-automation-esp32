/**
 * @file CommunicationsBase.h
 * @brief Abstract base class for ESP-NOW and WiFi communication
 * 
 * Derived classes implement device-specific message handling.
 * Uses a static callback to bridge ESP-NOW events to instance methods.
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

    void initializeWifi();
    bool initializeESPNOW();
    bool registerPeer(uint8_t* mac_address, uint8_t wifi_channel);
    bool unregisterPeer(uint8_t* mac_address);
    bool sendMsg(uint8_t* mac_addr, const uint8_t* data, size_t size);
    void sendAck(const uint8_t* mac_addr, MessageType acked_msg);
    void setQueue(QueueHandle_t queue);

protected:
    // Static callback to handle incoming ESP-NOW data; we use a static method 
    // and 'instance' pointer to dispatch to the correct instance method.
    static void onDataRecvStatic(const uint8_t* mac_addr, const uint8_t* data, int len);
    virtual void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) = 0;

    static CommunicationsBase* instance;

    QueueHandle_t dataQueue;

    struct Peer {
        uint8_t mac_addr[MAC_ADDRESS_LENGTH];
        esp_now_peer_info_t peer_info;
    };
    Peer peers[MAX_PEERS];
    int numPeers;

    SemaphoreHandle_t peerMutex; // Protects peer list against concurrent access
};
