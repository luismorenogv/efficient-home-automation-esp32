/**
 * @file master.ino
 * @brief Master ESP32 main code
 * 
 * @author Luis Moreno
 * @date November 8, 2024
 */

#include "master.h"
#include <WiFi.h>

// ----------------------------
// Global Variables
// ----------------------------

// Array to hold information of connected slaves
SlaveInfo slaves[MAX_SLAVES];
uint8_t slave_count = 0;

// Timer for generating random actions
unsigned long last_action_time = 0;
constexpr uint32_t ACTION_INTERVAL_MS = 30000; // 30 seconds

// ----------------------------
// Callback Function
// ----------------------------

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < 1) {
    Serial.println("Received empty message.");
    return;
  }

  MessageType msg_type = static_cast<MessageType>(incoming_data[0]);

  if (len != SIZE_OF[static_cast<uint8_t>(msg_type)]) {
    Serial.print("Received ");
    Serial.print(NAME[static_cast<uint8_t>(msg_type)]);
    Serial.println(" with incorrect length");
    return;
  }

  switch (msg_type) {
    case MessageType::IM_HERE:
    {
      Serial.println("Received IM_HERE from a slave.");
      // Extract payload
      const uint8_t *payload = incoming_data + 1; // Skip message type
      handleNewSlave(info->src_addr, payload);
    }
    break;

    case MessageType::SENSOR_DATA:
    {
      // Extract sensor data
      SensorData received_data = {};
      memcpy(&received_data, incoming_data + 1, sizeof(SensorData));

      uint8_t id = received_data.id;
      if (id < slave_count) {
        // Store data
        SlaveInfo &slave = slaves[id];
        slave.temperature[slave.data_index] = received_data.temperature;
        slave.humidity[slave.data_index] = received_data.humidity;

        // Send ACK
        MessageType acked_msg = MessageType::SENSOR_DATA;
        sendMsg(slave.mac_addr, MessageType::ACK, reinterpret_cast<uint8_t *>(&acked_msg), SIZE_OF[static_cast<uint8_t>(MessageType::ACK)]);

        // Print received sensor data
        Serial.print("Slave ID ");
        Serial.print(id);
        Serial.print(" - Temperature: ");
        Serial.print(received_data.temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(received_data.humidity);
        Serial.println(" %");

        // Update index
        slave.data_index = (slave.data_index + 1) % DATA_ARRAY_SIZE;
      } else {
        Serial.println("Received SENSOR_DATA from unknown slave ID.");
      }
    }
    break;

    case MessageType::REQ:
    {
      uint8_t slave_id = incoming_data[1]; // ID is in payload
      Serial.print("Received REQ from Slave ID ");
      Serial.println(slave_id);

      if (slave_id < slave_count) {
        handleReq(info->src_addr, slave_id);
      } else {
        Serial.println("Received REQ from unknown slave ID.");
      }
    }
    break;

    case MessageType::ACK:
    {
      MessageType acked_msg = static_cast<MessageType>(incoming_data[1]);
      Serial.print("Received ACK for message type: ");
      Serial.println(NAME[static_cast<uint8_t>(acked_msg)]);
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
    if (memcmp(slaves[i].mac_addr, slave_mac, MAC_ADDRESS_LENGTH) == 0) {
      Serial.println("Slave already acknowledged, resending START message");
      // Resend START message with ID
      uint8_t id_payload = slaves[i].id;
      sendMsg(slave_mac, MessageType::START, &id_payload, SIZE_OF[static_cast<uint8_t>(MessageType::START)]);
      return;
    }
  }

  // Add new slave as a peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, slave_mac, MAC_ADDRESS_LENGTH);
  peerInfo.channel = 0; 
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add slave as a peer");
    return;
  }

  // Add new slave to the list
  SlaveInfo &new_slave = slaves[slave_count];
  new_slave.id = slave_count; // Assign ID as the index
  memcpy(new_slave.mac_addr, slave_mac, MAC_ADDRESS_LENGTH);

  // Extract wake_up_period_ms and time_awake_ms from payload
  memcpy(&new_slave.wake_up_period_ms, payload, sizeof(uint32_t));
  memcpy(&new_slave.time_awake_ms, payload + sizeof(uint32_t), sizeof(uint32_t));

  // Initialize pending actions
  new_slave.action_index = 0;
  memset(new_slave.pending_actions, 0, sizeof(new_slave.pending_actions));

  slave_count++;

  Serial.print("New slave added with ID: ");
  Serial.println(new_slave.id);
  Serial.print("Total slaves: ");
  Serial.println(slave_count);

  // Send START message with ID
  uint8_t id_payload = new_slave.id;
  sendMsg(slave_mac, MessageType::START, &id_payload, SIZE_OF[static_cast<uint8_t>(MessageType::START)]);
}

void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload, size_t size) {
  esp_err_t result = ESP_OK;
  Message msg = {};
  msg.type = msg_type;
  memset(msg.payload, 0, sizeof(msg.payload)); // Clear payload

  if (payload != nullptr && size > sizeof(msg.type)) {
    memcpy(msg.payload, payload, size - sizeof(msg.type)); // Adjusted size
  }

  size_t msg_size = size > 0 ? size : SIZE_OF[static_cast<uint8_t>(msg_type)];

  result = esp_now_send(dest_mac, reinterpret_cast<uint8_t *>(&msg), msg_size);

  if (result == ESP_OK) {
    Serial.print(NAME[static_cast<uint8_t>(msg.type)]);
    Serial.println(" message sent successfully.");
  } else {
    Serial.print("Error sending message ");
    Serial.print(NAME[static_cast<uint8_t>(msg.type)]);
    Serial.print(": ");
    Serial.println(result);
  }
}

void handleReq(const uint8_t *slave_mac, uint8_t slave_id) {
  // Prepare ACTION message
  ActionMessage action_msg = {};
  action_msg.num_actions = slaves[slave_id].action_index;
  memcpy(action_msg.actions, slaves[slave_id].pending_actions, action_msg.num_actions);

  // Prepare the payload
  uint8_t payload[sizeof(ActionMessage)] = {};
  memcpy(payload, &action_msg, sizeof(ActionMessage));

  // Send ACTION message
  sendMsg(slave_mac, MessageType::ACTION, payload, SIZE_OF[static_cast<uint8_t>(MessageType::ACTION)]);

  Serial.print("Sent ACTION message to Slave ID ");
  Serial.print(slave_id);
  Serial.print(" with ");
  Serial.print(action_msg.num_actions);
  Serial.println(" actions.");

  // Clear pending actions
  slaves[slave_id].action_index = 0;
  memset(slaves[slave_id].pending_actions, 0, sizeof(slaves[slave_id].pending_actions));
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

void setup() {
  Serial.begin(115200);
  Serial.println("Master Setup");

  initializeEspNow();

  // Initialize slaves info to zero
  memset(&slaves, 0, sizeof(slaves));

  // Initialize random number generator
  randomSeed(esp_random());
}

void loop() {
  // Generate random actions every ACTION_INTERVAL_MS
  if (millis() - last_action_time >= ACTION_INTERVAL_MS) {
    last_action_time = millis();
    if (slave_count > 0) {
      // Select a random slave
      uint8_t random_slave = random(0, slave_count);
      // Select a random action
      ActionCode random_action = static_cast<ActionCode>(random(0, 2)); // Assuming two actions

      // Add action to the slave's pending actions
      SlaveInfo &slave = slaves[random_slave];
      if (slave.action_index < MAX_PENDING_ACTIONS) {
        slave.pending_actions[slave.action_index++] = static_cast<uint8_t>(random_action);
        Serial.print("Added action ");
        Serial.print(random_action == ActionCode::TURN_OFF_AC ? "TURN_OFF_AC" : "TURN_ON_AC");
        Serial.print(" to Slave ID ");
        Serial.println(random_slave);
      } else {
        Serial.print("Pending actions queue full for Slave ID ");
        Serial.println(random_slave);
      }
    } else {
      Serial.println("No slaves connected to assign actions.");
    }
  }

  delay(100); // Small delay to prevent tight loop
}


