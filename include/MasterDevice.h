/**
 * @file MasterDevice.h 
 * @brief Declaration of MasterDevice class for handling web server, WebSockets, ESPNOW communication, and NTP synchronization.
 * 
 * @author Luis Moreno
 * @date Nov 19, 2024
 */

#pragma once

#include "common.h"
#include "mac_addrs.h"
#include "secrets.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Number of rooms
constexpr uint8_t NUM_ROOMS = 3;

constexpr uint16_t MAX_DATA_POINTS = 300;

constexpr const char* ROOM_NAME[NUM_ROOMS] = {"Habitación Luis", "Habitación Pablo", "Habitación Ana"};

struct RoomData {
    float temperature[MAX_DATA_POINTS];
    float humidity[MAX_DATA_POINTS];
    time_t timestamps[MAX_DATA_POINTS];
    uint16_t index;
    uint16_t valid_data_points;

    RoomData() : index(0), valid_data_points(0) {
        memset(temperature, 0, sizeof(temperature));
        memset(humidity, 0, sizeof(humidity));
        memset(timestamps, 0, sizeof(timestamps));
    }
};



class MasterDevice {
private:
    // Singleton instance
    static MasterDevice* instance;

    // Wi-Fi and ESP-NOW
    esp_now_peer_info_t sensorPeer;

    // Web Server and WebSocket
    AsyncWebServer server;
    AsyncWebSocket ws;

    // Tasks and Queues
    TaskHandle_t espnowTaskHandle;
    TaskHandle_t webServerTaskHandle;
    QueueHandle_t espnowQueue;

    // Room data
    RoomData rooms[NUM_ROOMS];

    // Private constructor
    MasterDevice();

    // Initialization methods
    void initialize();
    void initializeWiFi();
    void initializeESPNow();
    void initializeNTP();

    // Task methods
    static void espnowTask(void* pvParameter);
    static void webServerTask(void* pvParameter);

    // ESP-NOW callback
    static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len);

    // Data handling
    void handleReceivedData(const uint8_t* mac_addr, const uint8_t* data, int len);
    void sendAck(const uint8_t* mac_addr, MessageType acked_msg);

    // Web server setup
    void setupWebServer();
    void setupWebSockets();

    // Send data via WebSocket
    void sendWebSocketData(uint8_t room_id, uint16_t data_point);

    // Send the latest data to plot
    void sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id);

public:
    // Get the singleton instance
    static MasterDevice* getInstance();

    // Start the MasterDevice
    void start();
};
