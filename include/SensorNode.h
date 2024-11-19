/**
 * @file SensorNode.h 
 * @brief Declaration of SensorNode class for handling sensor readings and ESPNOW communication.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#pragma once

#include "common.h"
#include "mac_addrs.h"
#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

// Maximum number of retries for sending data
constexpr uint8_t MAX_RETRIES = 3;

// Default wake interval in milliseconds (5 minutes)
constexpr unsigned long DEFAULT_WAKE_INTERVAL = 300000;

// Timeout for ACK in milliseconds
constexpr unsigned long ACK_TIMEOUT = 2000;

// DHT22 Pin Configuration
constexpr uint8_t DHT_PIN = 4; // GPIO pin connected to DHT22
constexpr uint8_t DHT_TYPE = DHT22;

// Forward declaration of AckMsg and TempHumidMsg from common.h
struct AckMsg;
struct TempHumidMsg;

class SensorNode {
private:
    uint8_t room_id;
    unsigned long wake_interval;
    DHT dht;

    // ESPNOW peer information
    esp_now_peer_info_t master_peer;

    // ACK received flag
    volatile bool ack_received;

    // Singleton instance
    static SensorNode* instance;

    // Static callback function for ESPNOW data reception
    static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len);

public:
    // Constructor with room ID
    SensorNode(uint8_t room_id);
    
    // Initializes the SensorNode (ESPNow, WiFi, DHT sensor)
    void initialize();
    
    // Sends sensor data to the MasterDevice
    void sendData();
    
    // Waits for an ACK from the MasterDevice with retries
    bool waitForAck();
    
    // Enters deep sleep mode for the defined wake interval
    void enterDeepSleep();
};

