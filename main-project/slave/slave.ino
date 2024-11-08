/**
 * @file slave.ino 
 * @brief Slave ESP32 main code
 * 
 * @author Luis Moreno
 * @date November 8, 2024
 */

#include "slave.h"
#include <WiFi.h>

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Flags and state variables
RTC_DATA_ATTR bool canOperate = false; // Flag indicating if the slave can start operating

// RTC memory attributes to retain data during deep sleep
RTC_DATA_ATTR uint8_t id = NO_ID;  // Slave ID assigned by the master

RTC_DATA_ATTR uint32_t wake_up_period_ms = DEFAULT_WAKE_UP_MS; // Default value
RTC_DATA_ATTR uint32_t time_awake_ms = DEFAULT_TIME_AWAKE_MS;  // Default value

RTC_DATA_ATTR uint16_t cycle_count = 0;

volatile unsigned long wake_up_instant = 0;
volatile bool dataAck = true;
unsigned long timer_start = 0;

volatile bool actionReceived = false;

// ----------------------------
// Callback Functions
// ----------------------------

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < 1) {
    Serial.println("Received empty message.");
    return;
  }

  MessageType msg_type = static_cast<MessageType>(incoming_data[0]);

  switch (msg_type) {
    case MessageType::START:
      if (len == SIZE_OF[static_cast<uint8_t>(MessageType::START)]) {
        id = incoming_data[1]; // ID is in payload
        Serial.print("Received START from master. Assigned ID: ");
        Serial.println(id);

        canOperate = true;
        wake_up_instant = millis();
      } else {
        Serial.println("Received START with incorrect length.");
      }
      break;

    case MessageType::ACTION:
      actionReceived = true;
      handleAction(incoming_data + 1, len - 1);
      break;

    case MessageType::ACK:
      if (len == SIZE_OF[static_cast<uint8_t>(MessageType::ACK)]) {
        MessageType acked_msg = static_cast<MessageType>(incoming_data[1]);
        if (acked_msg == MessageType::SENSOR_DATA) {
          dataAck = true;
          Serial.println("ACK received for SENSOR_DATA.");
        } else {
          Serial.print("ACK received for unknown message type: ");
          Serial.println(static_cast<uint8_t>(acked_msg), HEX);
        }
      } else {
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

void initializeEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    haltExecution();
  }
  Serial.println("ESP-NOW Initialized");

  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);

  // Configure peer (master)
  esp_now_peer_info_t peer_info = {};
  memcpy(peer_info.peer_addr, MASTER_ADDRESS, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add peer");
    haltExecution();
  }
}

void haltExecution() {
  Serial.println("Execution halted.");
  while (true) {
    delay(1000);
  }
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

SensorData readSensorData() {
  SensorData data = {};
  data.id = id; // Include the slave ID
  data.temperature = dht.readTemperature();
  data.humidity = dht.readHumidity();

  return data;
}

void sendImHereMessage() {
  uint8_t payload[sizeof(uint32_t) * 2];
  memcpy(payload, &wake_up_period_ms, sizeof(uint32_t));
  memcpy(payload + sizeof(uint32_t), &time_awake_ms, sizeof(uint32_t));
  sendMsg(MASTER_ADDRESS, MessageType::IM_HERE, payload, SIZE_OF[static_cast<uint8_t>(MessageType::IM_HERE)]);
}

void sendSensorData(const SensorData& data_to_send) {
  if (isnan(data_to_send.temperature) || isnan(data_to_send.humidity)) {
    Serial.println("DHT22 read failed");
    dataAck = true; // Avoid resending the message
  } else {
    dataAck = false;
    Serial.println("Sending SENSOR_DATA message.");
    sendMsg(MASTER_ADDRESS, MessageType::SENSOR_DATA, reinterpret_cast<const uint8_t *>(&data_to_send), SIZE_OF[static_cast<uint8_t>(MessageType::SENSOR_DATA)]);
    timer_start = millis(); // Start the timer after sending
  }
}

void handleAction(const uint8_t *payload, uint8_t payload_len) {
  if (payload_len < sizeof(ActionMessage)) {
    Serial.println("Invalid ACTION message received.");
    return;
  }

  ActionMessage action_msg = {};
  memcpy(&action_msg, payload, sizeof(ActionMessage));

  if (action_msg.num_actions > 0) {
    Serial.print("Received ACTION message with ");
    Serial.print(action_msg.num_actions);
    Serial.println(" actions:");

    for (uint8_t i = 0; i < action_msg.num_actions; i++) {
      ActionCode action = static_cast<ActionCode>(action_msg.actions[i]);
      switch (action) {
        case ActionCode::TURN_OFF_AC:
          Serial.println("Action: TURN_OFF_AC");
          // Implement the action here
          break;
        case ActionCode::TURN_ON_AC:
          Serial.println("Action: TURN_ON_AC");
          // Implement the action here
          break;
        default:
          Serial.print("Unknown action code: ");
          Serial.println(static_cast<uint8_t>(action));
          break;
      }
    }
  } else {
    Serial.println("No actions to perform.");
  }
}

void setup() {
  wake_up_instant = millis();
  Serial.begin(115200);
  Serial.print("Cycle: ");
  Serial.println(cycle_count);

  initializeEspNow();

  // Seed for random delays
  randomSeed(esp_random());
}

void loop() {
  if (!canOperate) {
    if (millis() - wake_up_instant > IM_HERE_TIMEOUT_MS) {
      Serial.println("Sending IM_HERE message.");
      sendImHereMessage();
      wake_up_instant = millis(); // Reset the timer after sending
    }
  } else {
    uint8_t retry_count = 0;
    bool req_sent = false;

    // Send REQ to master to request actions
    do {
      sendMsg(MASTER_ADDRESS, MessageType::REQ, &id, SIZE_OF[static_cast<uint8_t>(MessageType::REQ)]);
      req_sent = true;

      // Wait for ACTION message or until time_awake_ms expires
      unsigned long start_instant = millis();
      while (!actionReceived && (millis() - start_instant) < MSG_TIMEOUT_MS) {
        delay(50);
      }

      if (!actionReceived) {
        retry_count++;
        Serial.println("No ACTION received, retrying...");
        delay(random(100, 300)); // Random delay to avoid collisions
      }
    } while (!actionReceived && retry_count < MAX_RETRIES);

    if (!actionReceived) {
      Serial.println("Warning: No ACTION message received after maximum retries.");
    }

    // Send sensor data every SENSOR_DATA_PERIOD cycles
    if (cycle_count % SENSOR_DATA_PERIOD == 0) {
      Serial.println("Reading and sending temperature and humidity values...");
      dht.begin();
      SensorData data_to_send = readSensorData();

      retry_count = 0;
      dataAck = false;

      do {
        sendSensorData(data_to_send);

        // Wait for ACK or until MSG_TIMEOUT_MS expires
        unsigned long start_instant = millis();
        while (!dataAck && (millis() - start_instant) < MSG_TIMEOUT_MS) {
          delay(50);
        }

        if (!dataAck) {
          retry_count++;
          Serial.println("No ACK received for SENSOR_DATA, retrying...");
          delay(random(100, 300)); // Random delay to avoid collisions
        }
      } while (!dataAck && retry_count < MAX_RETRIES);

      if (!dataAck) {
        Serial.println("Warning: No SENSOR_DATA ACK received after maximum retries.");
      }
    } else {
      Serial.println("Sensor data not sent this cycle.");
    }

    // Increment cycle count
    cycle_count++;

    // Calculate sleep duration
    unsigned long elapsed_time = millis() - wake_up_instant;
    unsigned long sleep_duration = (wake_up_period_ms > elapsed_time + LEEWAY_MS) ? (wake_up_period_ms - elapsed_time - LEEWAY_MS) : 0;

    Serial.print("Sleeping for: ");
    Serial.print(sleep_duration);
    Serial.println(" millis.");

    // Configure next wake up
    esp_sleep_enable_timer_wakeup(sleep_duration * MS_TO_US);

    Serial.println("Entering Deep Sleep");
    esp_deep_sleep_start();
  }
}
