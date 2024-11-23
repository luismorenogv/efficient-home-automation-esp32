/**
 * @file WebSockets.h
 * @brief Declaration of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DataManager.h"

class WebSockets {
public:
    WebSockets(DataManager& dataManager);
    void initialize(AsyncWebServer& server);
    void sendDataUpdate(uint8_t room_id);
    void setPollingPeriodCallback(void (*callback)(uint8_t, uint32_t));

private:
    AsyncWebSocket ws;
    DataManager& dataManager;
    void (*pollingPeriodCallback)(uint8_t, uint32_t);

    void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                 void* arg, uint8_t* data, size_t len);
    void sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id);
};
