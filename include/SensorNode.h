/**
 * @file SensorNode.h 
 * @brief Declaration of SensorNode class for handling sensor readings and ESPNOW communication.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#ifndef SENSOR_NODE_H
#define SENSOR_NODE_H

#include "common.h"
#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

// Define master MAC address
const uint8_t master_mac[MAC_ADDRESS_LENGTH] = {0x24, 0x6f, 0x28, 0x2e, 0xa9, 0xc0};

// Maximum number of retries for sending data
constexpr uint8_t MAX_RETRIES = 3;

// Default wake interval in milliseconds (5 minutes)
constexpr unsigned long DEFAULT_WAKE_INTERVAL = 300000;

// Timeout for ACK in milliseconds
constexpr unsigned long ACK_TIMEOUT = 2000;

// DHT22 Pin Configuration
constexpr uint8_t DHT_PIN = 4; // GPIO pin connected to DHT22
constexpr uint8_t DHT_TYPE = DHT22;

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

    // Static callback function
    static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len);

public:
    SensorNode(uint8_t room_id);
    void initialize();
    void sendData();
    bool waitForAck();
    void enterDeepSleep();
};

#endif /* SENSOR_NODE_H */
