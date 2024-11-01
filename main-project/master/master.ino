/**
 * @file master.ino
 * @brief Master ESP32 main code
 * 
 * @author Luis Moreno
 * @date October 27, 2024
 */

#include "master.h"
#include <WiFi.h>

// ----------------------------
// Global Variables
// ----------------------------

// Arrays to store temperature and humidity data
float temperature_data[DATA_ARRAY_SIZE];
float humidity_data[DATA_ARRAY_SIZE];
int data_index = 0;  // Index to track data storage location

// Array to hold information of connected slaves
SlaveInfo slaves[MAX_SLAVES];
int slave_count = 0;

// ----------------------------
// Callback Function
// ----------------------------

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < 1) {
    Serial.println("Received empty message.");
    return;
  }

  MessageType msg_type = static_cast<MessageType>(incoming_data[0]);

  switch (msg_type) {
    case IM_HERE:
      Serial.println("Received IM_HERE from a new slave.");
      handleNewSlave(info -> src_addr);
      break;

    case SENSOR_DATA:
      if (len >= sizeof(MessageType) + sizeof(SensorData)) { // 1 byte type + 8 bytes data

        SensorData received_data;

        memcpy(&received_data, incoming_data + 1, sizeof(SensorData));

        // Print received sensor data
        Serial.print("Temperature: ");
        Serial.print(received_data.temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(received_data.humidity);
        Serial.println(" %");

        // Store data in arrays
        temperature_data[data_index] = received_data.temperature;
        humidity_data[data_index] = received_data.humidity;

        // Update index
        data_index = (data_index + 1) % DATA_ARRAY_SIZE;
        Serial.print("Data index updated to: ");
        Serial.println(data_index);
      } else {
        Serial.println("Received DATA with incorrect length.");
      }
      break;

    default:
      Serial.println("Received unknown or unhandled message type.");
      break;
  }
}



void haltExecution() {
  Serial.println("Execution halted");
  while (true) {
      delay(1000);  // Halt execution on failure
  }
}

void handleNewSlave(const uint8_t *slave_mac) {
  if (slave_count >= MAX_SLAVES) {
    Serial.println("Maximum number of slaves reached.");
    return;
  }

  // Verify if slave has been already added to the list
  for (uint8_t i = 0; i < slave_count; i++) {
    if (memcmp(slaves[i].mac_addr, slave_mac, 6) == 0){
      Serial.println("Slave already acknowledged, resending START msg");
      sendStartMsg(slave_mac);
      return;
    }
  }

  // Add new slave to the list
  memcpy(slaves[slave_count].mac_addr, slave_mac, 6);
  slaves[slave_count].last_sync_time = millis();
  slave_count++;

  Serial.print("New slave added. Total slaves: ");
  Serial.println(slave_count);

  sendStartMsg(slave_mac);
}


void sendStartMsg(const uint8_t *slave_mac){
  // Send START message to the new slave
  Message start_message;
  start_message.type = START;
  memset(start_message.payload, 0, sizeof(start_message.payload)); // No payload needed

  esp_err_t result = esp_now_send(slave_mac, (uint8_t *)&start_message, FRAME_SIZE[START]);

  if (result == ESP_OK) {
    Serial.println("START message sent successfully.");
  } else {
    Serial.println("Error sending START message.");
  }
}

void initializeEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    haltExecution();
  }
  Serial.println("ESP-NOW Initialized");

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  // Register the send callback
  // esp_now_register_send_cb(onDataSent);

  // Configure peer (slave)
  esp_now_peer_info_t peer_info;
  memset(&peer_info, 0, sizeof(peer_info)); // Initialize to zero
  memcpy(peer_info.peer_addr, SLAVE_ADDRESS, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add peer");
    haltExecution();
  }
}


// ----------------------------
// Setup Function
// ----------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("Master Setup");

    initializeEspNow();

    // Initialize slaves info to zero
    memset(&slaves, 0, sizeof(slaves));
}

// ----------------------------
// Loop Function
// ----------------------------

void loop() {
    // No actions needed in loop
}