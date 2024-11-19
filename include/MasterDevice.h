/**
 * @file MasterDevice.h 
 * @brief Declaration of MasterDevice class for handling web server and ESPNOW communication.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#pragma once

#include "common.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Number of rooms
constexpr uint8_t NUM_ROOMS = 3;

// Max temperature and humidity data points
constexpr uint16_t MAX_DATA_POINTS = 300;

// Structure to hold data for each room
struct RoomData {
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    uint16_t index;
    bool peer_added;

    RoomData() : index(0), peer_added(false) {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
    }
};

class MasterDevice {
private:
    AsyncWebServer server;
    bool ack_received;

    RoomData rooms[NUM_ROOMS]; 

    // Singleton instance
    static MasterDevice* instance;

    // Sets up the web server routes and handlers
    void setupWebServer();

    // Static callback function for ESPNOW data reception
    static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len);

public:
    MasterDevice();
    
    // Initializes the MasterDevice (ESPNow, WiFi, Web Server)
    void initialize();
    
    // Sends an ACK message to the specified peer
    void sendAck(const uint8_t *mac_addr, MessageType acked_msg);
    
    // Handles received data from sensor nodes
    void handleReceivedData(const uint8_t *mac_addr, const uint8_t *data, int len);
    
    // Updates web data and sends JSON response
    void updateWebData(AsyncWebServerRequest *request);
    
    // Registers a new peer with ESPNOW
    bool registerPeer(const uint8_t *mac_addr);
};

