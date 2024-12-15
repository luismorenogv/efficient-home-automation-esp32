/**
 * @file WebSockets.cpp
 * @brief Implementation of WebSockets class for handling WebSocket communication with clients
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "MasterDevice/WebSockets.h"

// Constructor initializes WebSocket path and callback pointers
WebSockets::WebSockets(DataManager& dataManager) : ws("/ws"), dataManager(dataManager),
        sleepDurationCallback(nullptr), scheduleCallback(nullptr) {
}

// Initializes WebSocket events and adds the handler to the server
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

void WebSockets::setScheduleCallback(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)) {
    scheduleCallback = callback;
}
// Handles WebSocket events such as connections, disconnections, and incoming data
void WebSockets::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                         void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            LOG_INFO("WebSocket client %u connected", client->id());

            // Send current sensor data for all registered rooms upon client connection
            for (uint8_t i = 0; i < NUM_ROOMS; i++) {
                const RoomData& room_data = dataManager.getRoomData(i);
                if (room_data.isRegistered()) {
                    sendDataUpdate(i);
                }
            }
            break;

        case WS_EVT_DISCONNECT:
            LOG_INFO("WebSocket client %u disconnected", client->id());
            break;

        case WS_EVT_DATA:
            {
                AwsFrameInfo* info = (AwsFrameInfo*)arg;
                // Ensure the message is final, not fragmented, and is text-based
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    String msg = String((char*)data).substring(0, len);
                    LOG_INFO("Received message from client %u: %s", client->id(), msg.c_str());

                    DynamicJsonDocument doc(128);
                    DeserializationError error = deserializeJson(doc, msg);
                    if (error) {
                        LOG_ERROR("Failed to parse JSON message from client.");
                        sendError(client, "Invalid JSON format");
                        return;
                    }

                    JsonObject root = doc.as<JsonObject>();
                    if (!root.containsKey("action")) {
                        LOG_ERROR("JSON message missing 'action' field.");
                        sendError(client, "Missing 'action' field'");
                        return;
                    }

                    String action = root["action"].as<String>();

                    if (action == "setSleepPeriod") {
                        handleSetSleepPeriod(client, root);
                    } else if (action == "getHistory") {
                        handleGetHistory(client, root);
                    } else if (action == "setSchedule") {
                        handleSetSchedule(client, root);

                    }
                }
            }
            break;

        default:
            break;
    }
}

void WebSockets::handleSetSleepPeriod(AsyncWebSocketClient* client, JsonObject& root) {
    if (!root.containsKey("room_id") || !root.containsKey("sleep_period")) {
        LOG_ERROR("setSleepPeriod action missing required fields.");
        sendError(client, "Missing 'room_id' or 'sleep_period'");
        return;
    }

    uint8_t room_id = root["room_id"];
    String periodStr = root["sleep_period"].as<String>();

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
        LOG_ERROR("Unknown sleep period received");
        sendError(client, "Unknown sleep period");
        return;
    }

    LOG_INFO("Setting sleep period for room %u to %u ms", room_id, period_ms);

    if (sleepDurationCallback) {
        sleepDurationCallback(room_id, period_ms);
    } else {
        LOG_ERROR("No callback defined");
        return;
    }

    DynamicJsonDocument respDoc(128);
    respDoc["status"] = "success";
    respDoc["room_id"] = room_id;
    respDoc["new_sleep_period_ms"] = period_ms;

    String respStr;
    serializeJson(respDoc, respStr);
    client->text(respStr);
}

void WebSockets::handleGetHistory(AsyncWebSocketClient* client, JsonObject& root) {
    if (!root.containsKey("room_id")) {
        LOG_ERROR("getHistory action missing 'room_id' field.");
        sendError(client, "Missing 'room_id' field");
        return;
    }

    uint8_t room_id = root["room_id"];
    LOG_INFO("Received getHistory request for room %u", room_id);
    sendHistoryData(client, room_id);
}

void WebSockets::handleSetSchedule(AsyncWebSocketClient* client, JsonObject& root) {
    if (!root.containsKey("room_id") || !root.containsKey("warm_time") || !root.containsKey("cold_time")) {
        LOG_ERROR("setSchedule action missing required fields.");
        DynamicJsonDocument respDoc(128);
        sendError(client, "Missing 'room_id', 'warm_time', or 'cold_time'");
        return;
    }

    uint8_t room_id = root["room_id"];
    String warmVal = root["warm_time"].as<String>();
    String coldVal = root["cold_time"].as<String>();

    uint8_t warmHour, warmMin, coldHour, coldMin;
    if (sscanf(warmVal.c_str(), "%hhu:%hhu", &warmHour, &warmMin) != 2 ||
        sscanf(coldVal.c_str(), "%hhu:%hhu", &coldHour, &coldMin) != 2) {
        LOG_ERROR("Invalid time format received.");
        sendError(client, "Invalid time format, use HH:MM");
        return;
    }

    // Check if roomNode is registered
    RoomData roomData = dataManager.getRoomData(room_id);
    if (!roomData.control.registered) {
        LOG_ERROR("RoomNode not registered, cannot set schedule.");
        sendError(client, "RoomNode not registered");
        return;
    }

    if (scheduleCallback) {
        scheduleCallback(room_id, warmHour, warmMin, coldHour, coldMin);
    } else{
        LOG_ERROR("No callback defined");
        return;
    }

    DynamicJsonDocument respDoc(128);
    respDoc["status"] = "success";
    respDoc["room_id"] = room_id;
    respDoc["warm"] = warmVal;
    respDoc["cold"] = coldVal;

    String respStr;
    serializeJson(respDoc, respStr);
    client->text(respStr);

    LOG_INFO("Requested NEW_SCHEDULE for room %u: Warm=%02u:%02u, Cold=%02u:%02u", room_id, warmHour, warmMin, coldHour, coldMin);
}


void WebSockets::sendDataUpdate(uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    RoomData room = dataManager.getRoomData(room_id);

    // If there's no data at all, skip
    if (!room.isRegistered()) return;

    // For sensor node data
    uint16_t idx = room.sensor.index == 0 ? MAX_DATA_POINTS - 1 : room.sensor.index - 1;
    float last_temp = (room.sensor.registered && room.sensor.valid_data_points > 0) ? room.sensor.temperature[idx] : 0;
    float last_humid = (room.sensor.registered && room.sensor.valid_data_points > 0) ? room.sensor.humidity[idx] : 0;
    time_t last_ts = (room.sensor.registered && room.sensor.valid_data_points > 0) ? room.sensor.timestamps[idx] : 0;
    uint32_t sleep_ms = room.sensor.registered ? room.sensor.sleep_period_ms : DEFAULT_SLEEP_DURATION;

    DynamicJsonDocument doc(512);
    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = "update";
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];

    // Include sensor data only if registered
    if (room.sensor.registered) {
        obj["temperature"] = last_temp;
        obj["humidity"] = last_humid;
        obj["timestamp"] = last_ts;
        obj["sleep_period_ms"] = sleep_ms;
        obj["registered"] = true;
    } else{
        obj["registered"] = false;
    }

    // Include control data only if RoomNode is registered
    if (room.control.registered) {
        // Format times as HH:MM strings
        char warm_str[6];
        snprintf(warm_str, sizeof(warm_str), "%02u:%02u", room.control.warm.hour, room.control.warm.min);
        char cold_str[6];
        snprintf(cold_str, sizeof(cold_str), "%02u:%02u", room.control.cold.hour, room.control.cold.min);

        obj["warm_time"] = warm_str;
        obj["cold_time"] = cold_str;
    }

    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);

    LOG_INFO("Sent data update via WebSocket for room %u", room_id);
}


void WebSockets::sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    RoomData room = dataManager.getRoomData(room_id);
    uint16_t count = room.sensor.valid_data_points;

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

        uint16_t idx = room.sensor.index;
        uint16_t startIdx = (idx + MAX_DATA_POINTS - count) % MAX_DATA_POINTS;

        for (uint16_t i = 0; i < count; ++i) {
            uint16_t index = (startIdx + i) % MAX_DATA_POINTS;

            float temp = room.sensor.temperature[index];
            float humid = room.sensor.humidity[index];
            time_t ts = room.sensor.timestamps[index];

            if (ts == 0) continue;
            
            tempArray.add(temp);
            humidArray.add(humid);
            timeArray.add(ts);
        }
        LOG_INFO("Sending history data: timestamps=%d, temperatures=%d, humidities=%d", timeArray.size(), tempArray.size(), humidArray.size());
    }

    String jsonString;
    serializeJson(doc, jsonString);
    client->text(jsonString);
}

void WebSockets::sendError(AsyncWebSocketClient* client, const char* message) {
    DynamicJsonDocument respDoc(128);
    respDoc["status"] = "error";
    respDoc["message"] = message;
    String respStr;
    serializeJson(respDoc, respStr);
    client->text(respStr);
}
