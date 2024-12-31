/**
 * @file WebSockets.h
 * @brief Declaration of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DataManager.h"

// Class to manage WebSocket connections and messaging
class WebSockets {
public:
    WebSockets(DataManager& dataManager);
    void initialize(AsyncWebServer& server);
    void sendDataUpdate(uint8_t room_id);
    void setSleepDurationCallback(void (*callback)(uint8_t, uint32_t));
    void setScheduleCallback(void (*callback)(uint8_t room_id, uint8_t warm_hour, 
                             uint8_t warm_min, uint8_t cold_hour, uint8_t cold_min));
    void setLightsToggleCallback(void (*callback)(uint8_t, bool));

private:
    AsyncWebSocket ws;                  // WebSocket instance
    DataManager& dataManager;           // Reference to DataManager
    void (*sleepDurationCallback)(uint8_t, uint32_t); // Callback for sleep period changes
    void (*scheduleCallback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t); // Callback for schedule changes
    void (*lightsToggleCallback)(uint8_t, bool);

    // Handles incoming WebSocket events
    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                void* arg, uint8_t* data, size_t len);

    // Processes the "setSleepPeriod" action
    void handleSetSleepPeriod(AsyncWebSocketClient* client, JsonObject& doc);

    // Processes the "getHistory" action
    void handleGetHistory(AsyncWebSocketClient* client, JsonObject& doc);

    // Processes the "setSchedule" action
    void handleSetSchedule(AsyncWebSocketClient* client, JsonObject& doc);

    // Sends historical data to a client for a specific room
    void sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id);

    // Manages lights toggle petition from the user
    void handleToggleLights(AsyncWebSocketClient* client, JsonObject& root);

    // Sends an error message to a client
    void sendError(AsyncWebSocketClient* client, const char* message);
};