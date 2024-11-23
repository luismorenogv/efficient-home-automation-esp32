/**
 * @file WebSockets.cpp
 * @brief Implementation of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#include "MasterDevice/WebSockets.h"

WebSockets::WebSockets(DataManager& dataManager) : ws("/ws"), dataManager(dataManager), pollingPeriodCallback(nullptr) {
}

void WebSockets::initialize(AsyncWebServer& server) {
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });

    server.addHandler(&ws);
}

void WebSockets::setPollingPeriodCallback(void (*callback)(uint8_t, uint32_t)) {
    pollingPeriodCallback = callback;
}

void WebSockets::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                         void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client %u connected\r\n", client->id());

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
                obj["wake_interval_ms"] = room.wake_interval_ms;

                String jsonString;
                serializeJson(doc, jsonString);

                client->text(jsonString);
            }
        }
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client %u disconnected\r\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            String msg = String((char*)data).substring(0, len);
            Serial.printf("Received message from client: %s\n", msg.c_str());

            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, msg);
            if (error) {
                Serial.println("Failed to parse JSON message");
                return;
            }

            if (doc.containsKey("action") && doc["action"].as<String>() == "setPollingPeriod") {
                uint8_t room_id = doc["room_id"];
                String periodStr = doc["polling_period"].as<String>(); // e.g., "5min", "15min", etc.

                uint32_t period_ms = 0;
                if (periodStr == "5min") {
                    period_ms = 5 * 60 * 1000;
                } else if (periodStr == "15min") {
                    period_ms = 15 * 60 * 1000;
                } else if (periodStr == "30min") {
                    period_ms = 30 * 60 * 1000;
                } else if (periodStr == "1h") {
                    period_ms = 60 * 60 * 1000;
                } else if (periodStr == "3h") {
                    period_ms = 3 * 60 * 60 * 1000;
                } else if (periodStr == "6h") {
                    period_ms = 6 * 60 * 60 * 1000;
                } else {
                    Serial.println("Unknown polling period received");
                    return;
                }

                Serial.printf("Setting polling period for room %u to %u ms\n", room_id, period_ms);

                if (pollingPeriodCallback) {
                    pollingPeriodCallback(room_id, period_ms);
                }

                // Send confirmation to the client
                DynamicJsonDocument respDoc(128);
                JsonObject respObj = respDoc.to<JsonObject>();
                respObj["status"] = "success";
                respObj["room_id"] = room_id;
                respObj["new_polling_period_ms"] = period_ms;

                String respStr;
                serializeJson(respDoc, respStr);
                client->text(respStr);
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

    uint16_t startIdx = (idx + MAX_DATA_POINTS - count) % MAX_DATA_POINTS;

    for (uint16_t i = 0; i < count; ++i) {
        uint16_t index = (startIdx + i) % MAX_DATA_POINTS;

        float temp = room.temperature[index];
        float humid = room.humidity[index];
        time_t ts = room.timestamps[index];

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
    uint16_t idx = room.index == 0 ? MAX_DATA_POINTS - 1 : room.index - 1;

    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];
    obj["temperature"] = room.temperature[idx];
    obj["humidity"] = room.humidity[idx];
    obj["timestamp"] = room.timestamps[idx];
    obj["wake_interval_ms"] = room.wake_interval_ms;

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);
    Serial.printf("Sent data update via WebSocket for room %u\r\n", room_id);
}
