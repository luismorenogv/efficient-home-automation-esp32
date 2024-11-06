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

// Array to hold information of connected slaves
SlaveInfo slaves[MAX_SLAVES];
uint8_t slave_count = 0;

// ----------------------------
// Callback Function
// ----------------------------

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < 1) {
    Serial.println("Received empty message.");
    return;
  }

  MessageType msg_type = static_cast<MessageType>(incoming_data[0]);

  if (len != SIZE_OF[msg_type]){
    Serial.print("Received ");
    Serial.print(NAME[msg_type]);
    Serial.println(" with incorrect length");
    return;
  }

  switch (msg_type) {
    case IM_HERE:
    {
      Serial.println("Received IM_HERE from a slave.");
      // Extract payload
      const uint8_t *payload = incoming_data + 1; // Skip message type
      handleNewSlave(info->src_addr, payload);
    }
    break;

    case SENSOR_DATA:
    {
      // Extract sensor data
      SensorData received_data;
      memcpy(&received_data, incoming_data + 1, SIZE_OF[msg_type] - sizeof(uint8_t));

      uint8_t id = received_data.id;
      if (id < slave_count) {
        // Store data
        slaves[id].temperature[slaves[id].data_index] = received_data.temperature;
        slaves[id].humidity[slaves[id].data_index] = received_data.humidity;

        // Send ACK
        MessageType acked_msg = SENSOR_DATA;
        sendMsg(slaves[id].mac_addr, ACK, (uint8_t*)&acked_msg, SIZE_OF[ACK]);

        // Print received sensor data
        Serial.print("Slave ID ");
        Serial.print(id);
        Serial.print(" - Temperature: ");
        Serial.print(received_data.temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(received_data.humidity);
        Serial.println(" %");

        // Update index
        slaves[id].data_index = (slaves[id].data_index + 1) % DATA_ARRAY_SIZE;
        Serial.print("Data index for Slave ID ");
        Serial.print(id);
        Serial.print(" updated to: ");
        Serial.println(slaves[id].data_index);
      } else {
        Serial.println("Received SENSOR_DATA from unknown slave ID.");
      }
    }
    break;

    case SYNC:
    {
      uint8_t id = incoming_data[1]; // ID está en payload
      if (id < slave_count) {
        if (slaves[id].last_sync_time != NOT_SYNCED){
          int32_t offset_error = slaves[id].wake_up_period_ms - ((millis() - slaves[id].last_sync_time) % slaves[id].wake_up_period_ms);
          Serial.print("Offset error for Slave ID ");
          Serial.print(id);
          Serial.print(": ");
          Serial.println(offset_error);
        }
        slaves[id].last_sync_time = millis();
        Serial.print("Received SYNC from slave ID ");
        Serial.println(id);
      } else {
        Serial.println("Received SYNC from unknown slave ID.");
      }
    }
    break;

    case ACK:
    {
      MessageType acked_msg = static_cast<MessageType>(incoming_data[1]);
      Serial.print("Received ACK for message type: ");
      Serial.println(NAME[acked_msg]);
    }
    break;

    default:
    {
      Serial.println("Received unknown or unhandled message type.");
    }
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

void handleNewSlave(const uint8_t *slave_mac, const uint8_t *payload) {
  if (slave_count >= MAX_SLAVES) {
    Serial.println("Maximum number of slaves reached.");
    return;
  }

  // Verify if slave has been already added to the list
  for (uint8_t i = 0; i < slave_count; i++) {
    if (memcmp(slaves[i].mac_addr, slave_mac, 6) == 0){
      Serial.println("Slave already acknowledged, resending START message");
      // Resend START message with ID
      uint8_t id_payload = slaves[i].id;
      sendMsg(slave_mac, START, &id_payload, SIZE_OF[START]);
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
  SlaveInfo &new_slave = slaves[slave_count];
  new_slave.id = slave_count; // Assign ID as the index
  memcpy(new_slave.mac_addr, slave_mac, 6);
  new_slave.last_sync_time = NOT_SYNCED;

  // Extract wake_up_period_ms and time_awake_ms from payload
  memcpy(&new_slave.wake_up_period_ms, payload, sizeof(uint32_t));
  memcpy(&new_slave.time_awake_ms, payload + sizeof(uint32_t), sizeof(uint32_t));

  slave_count++;

  Serial.print("New slave added with ID: ");
  Serial.println(new_slave.id);
  Serial.print("Total slaves: ");
  Serial.println(slave_count);

  // Send START message with ID
  uint8_t id_payload = new_slave.id;
  sendMsg(slave_mac, START, &id_payload, SIZE_OF[START]);
}

void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload, size_t size) {
  esp_err_t result = ESP_OK;
  Message msg;
  msg.type = msg_type;
  memset(msg.payload, 0, sizeof(msg.payload)); // Clear payload

  if (payload != nullptr && size > sizeof(msg.type)) {
    memcpy(msg.payload, payload, size - sizeof(msg.type)); // Adjusted size
  }

  size_t msg_size = size > 0 ? size : SIZE_OF[msg_type];

  result = esp_now_send(dest_mac, (uint8_t *)&msg, msg_size);

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
