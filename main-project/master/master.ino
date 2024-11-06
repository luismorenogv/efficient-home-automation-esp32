/**
 * @file master.ino
 * @brief Master ESP32 main code
 * 
 * @author Luis Moreno
 * @date November 1, 2024
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
      Serial.println("Received IM_HERE from a slave.");
      handleNewSlave(info->src_addr);
      break;

    case SYNC:
      Serial.println("Received SYNC from a slave.");
      // Update last_sync_time
      for (uint8_t i = 0; i < slave_count; i++) {
        if (memcmp(info->src_addr, slaves[i].mac_addr, sizeof(slaves[i].mac_addr)) == 0) {
          slaves[i].last_sync_time = millis();
          break;
        }
      }
      break;

    case SENSOR_DATA:
      if (len >= sizeof(MessageType) + sizeof(SensorData)) { // 1 byte type + 8 bytes data

        // Update last_sync_time for the slave
        bool slave_found = false;
        for (uint8_t i = 0; i < slave_count; i++) {
          if (memcmp(info->src_addr, slaves[i].mac_addr, sizeof(slaves[i].mac_addr)) == 0) {
            slaves[i].last_sync_time = millis();
            slave_found = true;
            break;
          }
        }

        if (!slave_found) {
          Serial.println("SENSOR_DATA received from an unknown slave.");
          break;
        }

        // Send ACK for SENSOR_DATA
        MessageType acked_msg = SENSOR_DATA;
        sendMsg(info->src_addr, ACK, (uint8_t*)&acked_msg);

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
        Serial.println("Received SENSOR_DATA with incorrect length.");
      }
      break;

    case ACK:
      if (len >= 2 * sizeof(MessageType)) { // Type + payload
        MessageType acked_msg = static_cast<MessageType>(incoming_data[1]);
        Serial.print("Received ACK for message type: ");
        Serial.println(NAME[acked_msg]);
        // Handle ACK if needed
      }
      else{
        Serial.println("ACK received with incorrect length.");
      }
      break;

    default:
      Serial.println("Received unknown or unhandled message type.");
      break;
  }
}

// ----------------------------
// Function Definitions
// ----------------------------

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
      Serial.println("Slave already acknowledged, resending START message");
      sendMsg(slave_mac, START);
      return;
    }
  }

  // Add new slave as a peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, slave_mac, 6);
  peerInfo.channel = 0; 
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add slave as a peer");
    return;
  }

  // Add new slave to the list
  memcpy(slaves[slave_count].mac_addr, slave_mac, 6);
  slaves[slave_count].last_sync_time = millis();
  slave_count++;

  Serial.print("New slave added. Total slaves: ");
  Serial.println(slave_count);

  sendMsg(slave_mac, START);
}

void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload) {
  esp_err_t result = ESP_OK;
  Message msg;
  msg.type = msg_type;
  switch (msg_type){
    case START:
      // Send START message to the new slave
      result = esp_now_send(dest_mac, (uint8_t *)&msg, SIZE_OF[msg_type]);
      break;
    case ACK:
      // Send ACK message with payload
      if (payload != nullptr){
        memcpy(msg.payload, payload, SIZE_OF[msg_type] - sizeof(msg.type)); // 1 byte
        result = esp_now_send(dest_mac, (uint8_t *)&msg, SIZE_OF[msg_type]);
      }
      else{
        Serial.println("ACK message requires payload.");
        return;
      }
      break;
    default:
      Serial.println("Attempted to send unknown message type.");
      return;
  }
  if (result == ESP_OK) {
    Serial.print(NAME[msg.type]);
    Serial.println(" message sent successfully.");
  } else {
    Serial.print("Error sending message ");
    Serial.print(NAME[msg.type]);
    Serial.print(": ");
    Serial.println(result);
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
}

void setup(){
  Serial.begin(115200);
  Serial.println("Master Setup");

  initializeEspNow();

  // Initialize slaves info to zero
  memset(&slaves, 0, sizeof(slaves));
}

void loop(){
  // Nothing here
}


