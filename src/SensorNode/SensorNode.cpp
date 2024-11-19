/**
 * @file SensorNode.cpp 
 * @brief Implementation of SensorNode class.
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */

#include "SensorNode.h"
#include <cstring>

// Initialize the static instance
SensorNode* SensorNode::instance = nullptr;

// Define the Master MAC address here to avoid multiple definitions
const uint8_t *master_mac = esp32s3_mac;

// Constructor
SensorNode::SensorNode(uint8_t room_id) 
    : room_id(room_id), 
      wake_interval(DEFAULT_WAKE_INTERVAL), 
      dht(DHT_PIN, DHT_TYPE), 
      ack_received(false) 
{
    // Assign the singleton instance
    instance = this;
}

// Static callback function for ESPNOW data reception
void SensorNode::onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);
    switch(msg_type){
        case MessageType::ACK:
        {
            if (len >= sizeof(AckMsg)) {
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
        default:
        {
            Serial.println("Received unknown or unhandled message type.");
        }  
        break;
    }
}

// Initialize Sensor Node
void SensorNode::initialize() {
    // Initialize Serial
    Serial.begin(115200);
    // Initialize DHT22 sensor
    dht.begin();

    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize ESPNOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESPNOW");
        return;
    }

    // Register master as peer
    memset(&master_peer, 0, sizeof(master_peer));
    memcpy(master_peer.peer_addr, master_mac, MAC_ADDRESS_LENGTH);
    master_peer.channel = 0;  
    master_peer.encrypt = false;

    if (esp_now_add_peer(&master_peer) != ESP_OK){
        Serial.println("Failed to add master as peer");
        return;
    }

    // Register receive callback
    esp_now_register_recv_cb(SensorNode::onDataRecvStatic);

    Serial.println("Sensor Node Initialized");
}

// Send Sensor Data to Master Node
void SensorNode::sendData() {
    TempHumidMsg msg;
    msg.type = MessageType::TEMP_HUMID;
    msg.room_id = room_id;
    msg.temperature = dht.readTemperature();
    msg.humidity = dht.readHumidity();

    if (isnan(msg.temperature) || isnan(msg.humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    esp_now_send(master_peer.peer_addr, reinterpret_cast<uint8_t*>(&msg), sizeof(TempHumidMsg));
    Serial.println("Sensor data sent to master");
}

// Wait for ACK with timeout and retries
bool SensorNode::waitForAck() {
    unsigned long start_time;
    uint8_t retries = 0;

    while (retries < MAX_RETRIES) {
        ack_received = false;
        sendData();
        start_time = millis();

        while ((millis() - start_time) < ACK_TIMEOUT) {
            if (ack_received) {
                return true;
            }
            delay(10); // Small delay to yield control
        }

        retries++;
        Serial.print("ACK not received, retrying... (");
        Serial.print(retries);
        Serial.println(")");
    }

    Serial.println("Failed to receive ACK after maximum retries");
    return false;
}

// Enter Deep Sleep
void SensorNode::enterDeepSleep() {
    Serial.println("Entering deep sleep");
    esp_deep_sleep_start();
}
