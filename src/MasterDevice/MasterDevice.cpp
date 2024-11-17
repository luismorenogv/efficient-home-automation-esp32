/**
 * @file MasterDevice.cpp 
 * @brief Implementation of MasterDevice class.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#include "MasterDevice.h"
#include <cstring>

// Inicializar la instancia estática
MasterDevice* MasterDevice::instance = nullptr;

// Constructor
MasterDevice::MasterDevice() : server(80), ack_received(false) {
    // Assign instance
    instance = this;
    memset(rooms, 0, sizeof(rooms));
}

// Static callback function
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

/// Handle Received Data from Sensor Nodes
void MasterDevice::handleReceivedData(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);

    switch(msg_type){
        case MessageType::ACK:
        {
            if (len = sizeof(AckMsg)) {
                const AckMsg *ack = reinterpret_cast<const AckMsg*>(data);

                instance->ack_received = true;

                Serial.print("ACK received from master for ");
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
                Serial.println("TEMP_HUMID msg received");
                const TempHumidMsg *temp_humid_msg = reinterpret_cast<const TempHumidMsg*>(data);
                uint8_t room_id = temp_humid_msg->room_id;
                if (room_id < NUM_ROOMS){
                    uint8_t index = rooms[room_id].index;
                    rooms[room_id].temperature[index] = temp_humid_msg->temperature;
                    rooms[room_id].humidity[index] = temp_humid_msg->humidity;

                    rooms[room_id].index = (index + 1) % MAX_DATA_POINTS;

                    if (!rooms[room_id].peer_added){
                        rooms[room_id].peer_added = registerPeer(mac_addr);
                    }
                    sendAck(mac_addr, msg_type);
                }
                else{
                    Serial.println("Received room id not valid");
                }
            }
            else {
                Serial.println("TEMP_HUMID_MSG received with incorrect length.");
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
    // Route for root / webpage
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateWebData(request);
    });
}

// Update Web Data
void MasterDevice::updateWebData(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    JsonArray sensorArray = doc.createNestedArray("habs");
    
    for (int i = 0; i < NUM_ROOMS; ++i) {
        JsonObject sensor = sensorArray.createNestedObject();
        sensor["hab_id"] = i;
        sensor["temperature"] = rooms[i].temperature[rooms[i].index];
        sensor["humidity"] = rooms[i].humidity[rooms[i].index];
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}
