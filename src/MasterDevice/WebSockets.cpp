/**
 * @file WebSockets.cpp
 * @brief Implementation of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#include "MasterDevice/WebSockets.h"

WebSockets::WebSockets(DataManager& dataManager) : ws("/ws"), dataManager(dataManager) {
}

void WebSockets::initialize(AsyncWebServer& server) {
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });

    server.addHandler(&ws);
}

void WebSockets::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                               void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client %u connected\r\n", client->id());

        // Send the latest data to the newly connected client
        for (uint8_t i = 0; i < NUM_ROOMS; ++i) {
            const RoomData& room = dataManager.getRoomData(i);
            if (room.valid_data_points > 0) {
                DynamicJsonDocument doc(256);
                JsonObject obj = doc.to<JsonObject>();
                uint16_t idx = room.index == 0 ? MAX_DATA_POINTS - 1 : room.index - 1;
                obj["room_id"] = i;
                obj["room_name"] = ROOM_NAME[i];
                obj["temperature"] = room.temperature[idx];
                obj["humidity"] = room.humidity[idx];
                obj["timestamp"] = room.timestamps[idx];

                String jsonString;
                serializeJson(doc, jsonString);

                client->text(jsonString);
            }
        }
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client %u disconnected\r\n", client->id());
    } else if (type == WS_EVT_DATA) {
        // Handle incoming messages
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            String msg = String((char*)data).substring(0, len);
            if (msg.startsWith("getHistory:")) {
                uint8_t room_id = msg.substring(strlen("getHistory:")).toInt();
                sendHistoryData(client, room_id);
            }
        }
    }
}

void WebSockets::sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    const size_t capacity = JSON_ARRAY_SIZE(MAX_DATA_POINTS) * 3 + JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);

    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = "history";
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];

    JsonArray tempArray = obj.createNestedArray("temperature");
    JsonArray humidArray = obj.createNestedArray("humidity");
    JsonArray timeArray = obj.createNestedArray("timestamps");

    const RoomData& room = dataManager.getRoomData(room_id);
    uint16_t count = room.valid_data_points;
    uint16_t idx = room.index;

    // Determine the starting index for the oldest data point
    uint16_t startIdx = (idx + MAX_DATA_POINTS - count) % MAX_DATA_POINTS;

    // Collect data in chronological order
    for (uint16_t i = 0; i < count; ++i) {
        uint16_t index = (startIdx + i) % MAX_DATA_POINTS;

        float temp = room.temperature[index];
        float humid = room.humidity[index];
        time_t ts = room.timestamps[index];

        // Skip invalid entries
        if (ts == 0) continue;

        tempArray.add(temp);
        humidArray.add(humid);
        timeArray.add(ts);
    }

    String jsonString;
    serializeJson(doc, jsonString);

    client->text(jsonString);
    Serial.printf("Sent history data to client %u for room %u\r\n", client->id(), room_id);
}

void WebSockets::sendDataUpdate(uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    const RoomData& room = dataManager.getRoomData(room_id);
    uint16_t idx = room.index == 0 ? MAX_DATA_POINTS - 1 : room.index - 1; // Latest data point

    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];
    obj["temperature"] = room.temperature[idx];
    obj["humidity"] = room.humidity[idx];
    obj["timestamp"] = room.timestamps[idx];

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);
    Serial.printf("Sent data update via WebSocket for room %u\r\n", room_id);
}
