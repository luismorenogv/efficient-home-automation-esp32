/**
 * @file MasterDevice.cpp 
 * @brief Implementation of MasterDevice class.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#include "MasterDevice.h"
#include <cstring>
#include "SPIFFS.h"

// Initialize the static instance
MasterDevice* MasterDevice::instance = nullptr;

// Constructor
MasterDevice::MasterDevice() : server(80), ack_received(false) {
    // Assign the instance
    instance = this;
}

// Static callback function for ESPNOW data reception
void MasterDevice::onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (instance) {
        instance->handleReceivedData(mac_addr, data, len);
    }
}

// Initialize Master Device
void MasterDevice::initialize() {
    // Initialize Serial
    Serial.begin(115200);
    
    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Connect to WiFi
    const char* ssid = WIFI_SSD;
    const char* password = WIFI_PASSWORD;

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi: ");
    Serial.print(ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    // Initialize ESPNOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESPNOW");
        return;
    }
    
    // Register callback for receiving data
    esp_now_register_recv_cb(MasterDevice::onDataRecvStatic);
    
    // Setup Web Server
    setupWebServer();
    
    // Start Web Server
    server.begin();
    Serial.println("Master Device Initialized and Web Server Started");
}

// Register a new peer with ESPNOW
bool MasterDevice::registerPeer(const uint8_t *mac_addr){
    esp_now_peer_info_t new_peer;

    memset(&new_peer, 0, sizeof(new_peer));
    memcpy(new_peer.peer_addr, mac_addr, MAC_ADDRESS_LENGTH);
    new_peer.channel = 0;  
    new_peer.encrypt = false;

    if (esp_now_add_peer(&new_peer) != ESP_OK){
        Serial.println("Failed to add new peer");
        return false;
    }
    return true;
}

// Handle Received Data from Sensor Nodes
void MasterDevice::handleReceivedData(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);

    switch(msg_type){
        case MessageType::ACK:
        {
            if (len == sizeof(AckMsg)) { // Correct comparison
                const AckMsg *ack = reinterpret_cast<const AckMsg*>(data);

                ack_received = true;

                Serial.print("ACK received from sensor for ");
                Serial.print(NAME[static_cast<uint8_t>(ack->acked_msg)]);
                Serial.println(" message");
            }
            else {
                Serial.println("ACK received with incorrect length.");
            }
        }
        break;
        case MessageType::TEMP_HUMID:
        {
            if (len == sizeof(TempHumidMsg)){
                Serial.println("TEMP_HUMID message received");
                const TempHumidMsg *temp_humid_msg = reinterpret_cast<const TempHumidMsg*>(data);
                uint8_t room_id = temp_humid_msg->room_id;
                if (room_id < NUM_ROOMS){
                    uint16_t index = rooms[room_id].index;
                    rooms[room_id].temperature[index] = temp_humid_msg->temperature;
                    rooms[room_id].humidity[index] = temp_humid_msg->humidity;

                    rooms[room_id].index = (index + 1) % MAX_DATA_POINTS;

                    if (!rooms[room_id].peer_added){
                        rooms[room_id].peer_added = registerPeer(mac_addr);
                    }
                    sendAck(mac_addr, msg_type);
                }
                else{
                    Serial.println("Received invalid room ID.");
                }
            }
            else {
                Serial.println("TEMP_HUMID message received with incorrect length.");
            }
        }
        break;
        default:
        {
            Serial.println("Received unknown or unhandled message type.");
        }  
        break;
    }
}

// Send ACK to Sensor Node
void MasterDevice::sendAck(const uint8_t *mac_addr, MessageType acked_msg) {
    AckMsg ack;
    ack.type = MessageType::ACK;
    ack.acked_msg = acked_msg;
    
    esp_now_send(mac_addr, reinterpret_cast<uint8_t*>(&ack), sizeof(AckMsg));
}

// Setup Web Server Routes
void MasterDevice::setupWebServer() {
    // Serve index.html at root
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    // Serve data.json at /data
    server.on("/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateWebData(request);
    });

    // Serve style.css
    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    // Serve script.js
    server.on("/script.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/script.js", "application/javascript");
    });
}

// Update Web Data and Send JSON Response
void MasterDevice::updateWebData(AsyncWebServerRequest *request) {
    const uint16_t SEND_POINTS = 100; // Number of data points to send
    DynamicJsonDocument doc(4096); // Adjust size if necessary
    JsonArray roomsArray = doc.createNestedArray("rooms");
    
    for (uint8_t i = 0; i < NUM_ROOMS; ++i) {
        JsonObject room = roomsArray.createNestedObject();
        room["room_id"] = i;
        JsonArray tempArray = room.createNestedArray("temperature");
        JsonArray humidArray = room.createNestedArray("humidity");
        
        // Calculate the starting index for the last SEND_POINTS
        uint16_t start = (rooms[i].index >= SEND_POINTS) ? (rooms[i].index - SEND_POINTS) : 0;
        uint16_t end = rooms[i].index;

        for (uint16_t j = start; j < end; ++j) {
            uint16_t idx = j % MAX_DATA_POINTS;
            tempArray.add(rooms[i].temperature[idx]);
            humidArray.add(rooms[i].humidity[idx]);
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

