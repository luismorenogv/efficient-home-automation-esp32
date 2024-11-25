/**
 * @file WebSockets.cpp
 * @brief Implementation of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/WebSockets.h"

WebSockets::WebSockets(DataManager& dataManager) : ws("/ws"), dataManager(dataManager), sleepDurationCallback(nullptr) {
}

void WebSockets::initialize(AsyncWebServer& server) {
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });

    server.addHandler(&ws);
}

void WebSockets::setSleepDurationCallback(void (*callback)(uint8_t, uint32_t)) {
    sleepDurationCallback = callback;
}

void WebSockets::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                         void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client %u connected\r\n", client->id());

            // Send current sensor data for all registered rooms upon client connection
            for (uint8_t i = 0; i < NUM_ROOMS; i++) {
                const RoomData& room_data = dataManager.getRoomData(i);
                if (room_data.registered) {
                    sendDataUpdate(i);
                }
            }
            break;

        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client %u disconnected\r\n", client->id());
            break;

        case WS_EVT_DATA:
            {
                AwsFrameInfo* info = (AwsFrameInfo*)arg;
                // Ensure the message is final, not fragmented, and is text-based
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    String msg = String((char*)data).substring(0, len);
                    Serial.printf("Received message from client %u: %s\r\n", client->id(), msg.c_str());

                    // Parse JSON message
                    DynamicJsonDocument doc(512); // Increased capacity for larger messages
                    DeserializationError error = deserializeJson(doc, msg);
                    if (error) {
                        Serial.println("Failed to parse JSON message from client.");
                        // Send error response to client
                        DynamicJsonDocument respDoc(128);
                        respDoc["status"] = "error";
                        respDoc["message"] = "Invalid JSON format";
                        String respStr;
                        serializeJson(respDoc, respStr);
                        client->text(respStr);
                        break;
                    }

                    // Validate 'action' field
                    if (!doc.containsKey("action")) {
                        Serial.println("JSON message missing 'action' field.");
                        // Send error response to client
                        DynamicJsonDocument respDoc(128);
                        respDoc["status"] = "error";
                        respDoc["message"] = "Missing 'action' field";
                        String respStr;
                        serializeJson(respDoc, respStr);
                        client->text(respStr);
                        break;
                    }

                    String action = doc["action"].as<String>();

                    if (action == "setSleepPeriod") {
                        // Handle setSleepPeriod action
                        if (!doc.containsKey("room_id") || !doc.containsKey("sleep_period")) {
                            Serial.println("setSleepPeriod action missing required fields.");
                            // Send error response to client
                            DynamicJsonDocument respDoc(128);
                            respDoc["status"] = "error";
                            respDoc["message"] = "Missing 'room_id' or 'sleep_period'";
                            String respStr;
                            serializeJson(respDoc, respStr);
                            client->text(respStr);
                            break;
                        }

                        uint8_t room_id = doc["room_id"];
                        String periodStr = doc["sleep_period"].as<String>();

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
                            Serial.println("Unknown sleep period received");
                            // Send error response to client
                            DynamicJsonDocument respDoc(128);
                            respDoc["status"] = "error";
                            respDoc["message"] = "Unknown sleep period";
                            String respStr;
                            serializeJson(respDoc, respStr);
                            client->text(respStr);
                            break;
                        }

                        Serial.printf("Setting sleep period for room %u to %u ms\r\n", room_id, period_ms);

                        if (sleepDurationCallback) {
                            sleepDurationCallback(room_id, period_ms);
                        }

                        // Send confirmation to the client
                        DynamicJsonDocument respDoc(128);
                        respDoc["status"] = "success";
                        respDoc["room_id"] = room_id;
                        respDoc["new_sleep_period_ms"] = period_ms;

                        String respStr;
                        serializeJson(respDoc, respStr);
                        client->text(respStr);

                    } else if (action == "getHistory") {
                        // Handle getHistory action
                        if (!doc.containsKey("room_id")) {
                            Serial.println("getHistory action missing 'room_id' field.");
                            // Send error response to client
                            DynamicJsonDocument respDoc(128);
                            respDoc["status"] = "error";
                            respDoc["message"] = "Missing 'room_id' field";
                            String respStr;
                            serializeJson(respDoc, respStr);
                            client->text(respStr);
                            break;
                        }

                        uint8_t room_id = doc["room_id"];
                        Serial.printf("Received getHistory request for room %u\r\n", room_id);
                        sendHistoryData(client, room_id);

                    } else {
                        Serial.printf("Unknown action received: %s\r\n", action.c_str());
                        // Send error response to client
                        DynamicJsonDocument respDoc(128);
                        respDoc["status"] = "error";
                        respDoc["message"] = "Unknown action";
                        String respStr;
                        serializeJson(respDoc, respStr);
                        client->text(respStr);
                    }

                } else {
                    Serial.println("Received non-text or fragmented WebSocket message.");
                }
            }
            break;

        default:
            break;
    }
}


void WebSockets::sendDataUpdate(uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    RoomData room = dataManager.getRoomData(room_id);
    uint16_t idx = room.index == 0 ? MAX_DATA_POINTS - 1 : room.index - 1;

    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = "update";
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];
    obj["temperature"] = room.temperature[idx];
    obj["humidity"] = room.humidity[idx];
    obj["timestamp"] = room.timestamps[idx];
    obj["sleep_period_ms"] = room.sleep_period_ms;

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);
    Serial.printf("Sent data update via WebSocket for room %u\r\n", room_id);
}


void WebSockets::sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    RoomData room = dataManager.getRoomData(room_id);
    uint16_t count = room.valid_data_points;

    DynamicJsonDocument doc(1024);
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = "history";
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];

    if (count == 0) {
        obj["message"] = "No historical data available.";
    } else {
        JsonArray tempArray = obj.createNestedArray("temperature");
        JsonArray humidArray = obj.createNestedArray("humidity");
        JsonArray timeArray = obj.createNestedArray("timestamps");

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
    }

    String jsonString;
    serializeJson(doc, jsonString);

    client->text(jsonString);
    Serial.printf("Sent history data to client %u for room %u\r\n", client->id(), room_id);
}

