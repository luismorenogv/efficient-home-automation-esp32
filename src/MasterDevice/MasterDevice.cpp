/**
 * @file MasterDevice.cpp 
 * @brief Implementation of MasterDevice class with WebSockets and NTP synchronization.
 * 
 * @author Luis Moreno
 * @date Nov 19, 2024
 */

#include "MasterDevice.h"
#include <SPIFFS.h>

// Initialize the static instance
MasterDevice* MasterDevice::instance = nullptr;

// Constructor
MasterDevice::MasterDevice() : server(80), ws("/ws") {
    instance = this;
}

// Get the singleton instance
MasterDevice* MasterDevice::getInstance() {
    if (instance == nullptr) {
        instance = new MasterDevice();
    }
    return instance;
}

// Start the MasterDevice
void MasterDevice::start() {
    initialize();
}

// Initialization
void MasterDevice::initialize() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    // Initialize Wi-Fi
    initializeWiFi();

    // Initialize NTP (Wait for time synchronization)
    initializeNTP();

    // Initialize ESP-NOW
    initializeESPNow();

    // Create queues
    espnowQueue = xQueueCreate(10, sizeof(TempHumidMsg));

    // Create tasks
    xTaskCreatePinnedToCore(
        espnowTask,         // Task function
        "ESP-NOW Task",     // Name
        4096,               // Stack size
        this,               // Parameter
        2,                  // Priority
        &espnowTaskHandle,  // Task handle
        0                   // Core 0
    );

    xTaskCreatePinnedToCore(
        webServerTask,      // Task function
        "Web Server Task",  // Name
        8192,               // Stack size
        this,               // Parameter
        1,                  // Priority
        &webServerTaskHandle,// Task handle
        0                   // Core 0
    );
}

// Initialize Wi-Fi
void MasterDevice::initializeWiFi() {
    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false); // Disable Wi-Fi sleep

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Print Wi-Fi Channel
    uint8_t wifi_channel = WiFi.channel();
    Serial.print("Wi-Fi Channel: ");
    Serial.println(wifi_channel);
}

// Initialize ESP-NOW
void MasterDevice::initializeESPNow() {
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register callback
    esp_now_register_recv_cb(MasterDevice::onDataRecv);

    // Register SensorNode as peer
    memset(&sensorPeer, 0, sizeof(sensorPeer));
    const uint8_t *sensor_mac = esp32dev_mac;
    memcpy(sensorPeer.peer_addr, sensor_mac, MAC_ADDRESS_LENGTH);
    sensorPeer.channel = 0; // Use current Wi-Fi channel
    sensorPeer.encrypt = false;

    if (esp_now_add_peer(&sensorPeer) == ESP_OK) {
        Serial.println("SensorNode added as peer successfully");
    } else {
        Serial.println("Failed to add SensorNode as peer");
    }
}

// Initialize NTP
void MasterDevice::initializeNTP() {
    configTime(3600 * 1, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;

    // Wait until time is synchronized
    int retry = 0;
    const int retry_count = 10;
    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        Serial.println("Waiting for NTP time synchronization");
        delay(2000); // Wait for 2 seconds before retrying
        retry++;
    }

    if (retry == retry_count) {
        Serial.println("Failed to synchronize time after multiple attempts");
    } else {
        time_t now = time(nullptr);
        Serial.printf("Time synchronized: %ld\r\n", now);
        Serial.println(&timeinfo, "Current time: %A, %B %d %Y %H:%M:%S");
    }
}



// ESP-NOW receive callback
void MasterDevice::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (instance) {
        instance->handleReceivedData(mac_addr, data, len);
    }
}

// Handle received data
void MasterDevice::handleReceivedData(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (len != sizeof(TempHumidMsg)) {
        Serial.println("Received data of incorrect size");
        return;
    }

    TempHumidMsg msg;
    memcpy(&msg, data, sizeof(TempHumidMsg));

    // Enqueue the message
    if (xQueueSendFromISR(espnowQueue, &msg, NULL) != pdTRUE) {
        Serial.println("Failed to enqueue ESP-NOW message");
    }
}

// ESP-NOW Task
void MasterDevice::espnowTask(void* pvParameter) {
    MasterDevice* self = static_cast<MasterDevice*>(pvParameter);
    TempHumidMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {

            uint8_t room_id = msg.room_id;
            if (room_id < NUM_ROOMS) {

                uint16_t idx = self->rooms[room_id].index;
                self->rooms[room_id].temperature[idx] = msg.temperature;
                self->rooms[room_id].humidity[idx] = msg.humidity;
                self->rooms[room_id].timestamps[idx] = time(nullptr);
                self->rooms[room_id].valid_data_points++;
                if (self->rooms[room_id].valid_data_points > MAX_DATA_POINTS) {
                    self->rooms[room_id].valid_data_points = MAX_DATA_POINTS;
                }

                // Update index
                self->rooms[room_id].index = (idx + 1) % MAX_DATA_POINTS;

                // Send ACK
                self->sendAck(self->sensorPeer.peer_addr, MessageType::TEMP_HUMID);

                // Send data via WebSocket
                self->sendWebSocketData(room_id, idx);
            }
        }
    }
}



// Send ACK
void MasterDevice::sendAck(const uint8_t* mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;

    esp_err_t result = esp_now_send(mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(AckMsg));
    if (result == ESP_OK) {
        Serial.println("ACK sent to sensor successfully");
    } else {
        Serial.printf("Error sending ACK: %d\r\n", result);
    }
}

// Web Server Task
void MasterDevice::webServerTask(void* pvParameter) {
    MasterDevice* self = static_cast<MasterDevice*>(pvParameter);

    // Setup web server and WebSocket
    self->setupWebServer();
    self->setupWebSockets();

    // Start the server
    self->server.begin();
    Serial.println("Web server started");

    // Run the server
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Setup Web Server
void MasterDevice::setupWebServer() {
    // Serve index.html at root
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    // Serve style.css
    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    // Serve script.js
    server.on("/script.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/script.js", "application/javascript");
    });
}

void MasterDevice::setupWebSockets() {
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket client %u connected\r\n", client->id());

            // Send the latest data to the newly connected client
            for (uint8_t i = 0; i < NUM_ROOMS; ++i) {
                if (rooms[i].valid_data_points > 0){
                    DynamicJsonDocument doc(256);
                    JsonObject obj = doc.to<JsonObject>();
                    uint16_t idx = rooms[i].index == 0 ? MAX_DATA_POINTS - 1 : rooms[i].index - 1;
                    obj["room_id"] = i;
                    obj["room_name"] = ROOM_NAME[i];
                    obj["temperature"] = rooms[i].temperature[idx];
                    obj["humidity"] = rooms[i].humidity[idx];
                    obj["timestamp"] = rooms[i].timestamps[idx];

                    String jsonString;
                    serializeJson(doc, jsonString);

                    client->text(jsonString);
                }
            }
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("WebSocket client %u disconnected\r\n", client->id());
        } else if (type == WS_EVT_DATA) {
            // Handle incoming messages
            AwsFrameInfo * info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                String msg = String((char*)data).substring(0, len);
                if (msg.startsWith("getHistory:")) {
                    uint8_t room_id = msg.substring(strlen("getHistory:")).toInt();
                    sendHistoryData(client, room_id);
                }
            }
        }
    });

    server.addHandler(&ws);
}


void MasterDevice::sendHistoryData(AsyncWebSocketClient* client, uint8_t room_id) {
    if (room_id >= NUM_ROOMS) return;

    const size_t capacity = JSON_ARRAY_SIZE(MAX_DATA_POINTS) * 3 + JSON_OBJECT_SIZE(5);
    DynamicJsonDocument doc(capacity);

    JsonObject obj = doc.to<JsonObject>();
    obj["type"] = "history";
    obj["room_id"] = room_id;

    JsonArray tempArray = obj.createNestedArray("temperature");
    JsonArray humidArray = obj.createNestedArray("humidity");
    JsonArray timeArray = obj.createNestedArray("timestamps");

    uint16_t count = rooms[room_id].valid_data_points;
    uint16_t idx = rooms[room_id].index;

    // Determine the starting index for the oldest data point
    uint16_t startIdx = (idx + MAX_DATA_POINTS - count) % MAX_DATA_POINTS;

    // Collect data in chronological order
    for (uint16_t i = 0; i < count; ++i) {
        uint16_t index = (startIdx + i) % MAX_DATA_POINTS;

        float temp = rooms[room_id].temperature[index];
        float humid = rooms[room_id].humidity[index];
        time_t ts = rooms[room_id].timestamps[index];

        // Skip invalid entries
        if (ts == 0) continue;

        tempArray.add(temp);
        humidArray.add(humid);
        timeArray.add(ts);
    }

    // Debugging: Print data arrays
    Serial.printf("Sending history data for Room ID %u: count = %u\r\n", room_id, count);
    for (size_t i = 0; i < tempArray.size(); ++i) {
        Serial.printf("Data point %u: Temp = %.2f, Humid = %.2f, Timestamp = %ld\r\n",
                      (unsigned int)i, tempArray[i].as<float>(), humidArray[i].as<float>(), timeArray[i].as<unsigned long>());
    }

    String jsonString;
    serializeJson(doc, jsonString);

    client->text(jsonString);
    Serial.printf("Sent history data to client %u for room %u\r\n", client->id(), room_id);
}


// Send data via WebSocket
void MasterDevice::sendWebSocketData(uint8_t room_id, uint16_t data_point) {

    DynamicJsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["room_id"] = room_id;
    obj["room_name"] = ROOM_NAME[room_id];
    obj["temperature"] = rooms[room_id].temperature[data_point];
    obj["humidity"] = rooms[room_id].humidity[data_point];
    obj["timestamp"] = rooms[room_id].timestamps[data_point];

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);
    Serial.println("Sent data via WebSocket");
    
    // Debugging: Print data with correct timestamps
    Serial.printf("Sending update data for Room ID %u with data point %u: Temp = %.2f, Humid = %.2f, Timestamp = %ld\r\n", 
                  room_id, data_point, rooms[room_id].temperature[data_point], rooms[room_id].humidity[data_point], rooms[room_id].timestamps[data_point]);
}

