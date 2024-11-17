/**
 * @file MasterDevice.h 
 * @brief Declaration of MasterDevice class for handling web server and ESPNOW communication.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#ifndef MASTER_DEVICE_H
#define MASTER_DEVICE_H

#include "common.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Number of rooms
constexpr uint8_t NUM_ROOMS = 3;

// Max temperature and humidity data points
constexpr uint16_t MAX_DATA_POINTS = 300;

struct roomData{
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    uint16_t index;
    bool peer_added;
} __attribute__((packed));

class MasterDevice {
private:
    AsyncWebServer server;
    bool ack_received;

    roomData rooms[NUM_ROOMS]; 

    // Singleton instance
    static MasterDevice* instance;

    void setupWebServer();

    // Static callback function
    static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len);

public:
    MasterDevice();
    void initialize();
    void sendAck(const uint8_t *mac_addr, MessageType acked_msg);
    void handleReceivedData(const uint8_t *mac_addr, const uint8_t *data, int len);
    void updateWebData(AsyncWebServerRequest *request);
    bool registerPeer(const uint8_t *mac_addr);
};

#endif /* MASTER_DEVICE_H */
