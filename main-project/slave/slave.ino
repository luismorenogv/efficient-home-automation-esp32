/**
 * @file slave.ino 
 * @brief Slave ESP32 main code
 *
 * @author Luis Moreno
 * @date November 1, 2024
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
RTC_DATA_ATTR uint32_t time_awake_ms = DEFAULT_TIME_AWAKE_MS;       // Default value

RTC_DATA_ATTR uint16_t cycle_count = 0;

volatile unsigned long wake_up_instant = 0;
volatile bool dataAck = true;
unsigned long timer_start = 0;

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
    case START:
      if (len == SIZE_OF[START]) {
        id = incoming_data[1]; // ID is in payload
        Serial.print("Received START from master. Assigned ID: ");
        Serial.println(id);

        canOperate = true;
        wake_up_instant = millis();

        // Send SYNC to master
        sendSyncMessage();

        Serial.print("Received START from master. Assigned ID: ");
        Serial.println(id);
      } else {
        Serial.println("Received START with incorrect length.");
      }
      break;

    case ACK:
      if (len == SIZE_OF[ACK]) {
        MessageType acked_msg = static_cast<MessageType>(incoming_data[1]);
        if (acked_msg == SENSOR_DATA){
          dataAck = true;
          Serial.println("ACK received for SENSOR_DATA.");
        } else {
          Serial.print("ACK received for unknown message type: ");
          Serial.println(acked_msg, HEX);
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
  // esp_now_register_send_cb(onDataSent);

  // Configure peer (master)
  esp_now_peer_info_t peer_info;
  memset(&peer_info, 0, sizeof(peer_info)); // Initialize to zero
  memcpy(peer_info.peer_addr, MASTER_ADDRESS, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add peer");
    haltExecution();
  }
}

void haltExecution(){
  Serial.println("Execution halted.");
  while (true) {
    delay(1000);
  }
}

void sendMsg(const uint8_t *dest_mac, MessageType msg_type, const uint8_t *payload) {
  esp_err_t result = ESP_OK;
  Message msg;
  msg.type = msg_type;
  memset(msg.payload, 0, sizeof(msg.payload)); // Clear payload

  if (payload != nullptr) {
    memcpy(msg.payload, payload, SIZE_OF[msg_type] - sizeof(uint8_t)); // Adjusted size
  }

  result = esp_now_send(dest_mac, (uint8_t *)&msg, SIZE_OF[msg_type]);

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

SensorData readSensorData(){
  SensorData data;
  data.id = id; // Include the slave ID
  data.temperature = dht.readTemperature();
  data.humidity = dht.readHumidity();

  return data;
}

void sendImHereMessage() {
  uint8_t payload[8];
  memcpy(payload, &wake_up_period_ms, sizeof(uint32_t));
  memcpy(payload + sizeof(uint32_t), &time_awake_ms, sizeof(uint32_t));
  sendMsg(MASTER_ADDRESS, IM_HERE, payload);
}

void sendSensorData() {
  SensorData data_to_send = readSensorData();

  if (isnan(data_to_send.temperature) || isnan(data_to_send.humidity)) {
    Serial.println("DHT22 read failed");
    dataAck = true; // Avoid resending the message
  } else {
    dataAck = false;
    Serial.println("Sending SENSOR_DATA message.");
    sendMsg(MASTER_ADDRESS, SENSOR_DATA, (uint8_t*)&data_to_send);
    timer_start = millis(); // Start the timer after sending
  }
}

void sendSyncMessage() {
  sendMsg(MASTER_ADDRESS, SYNC, (uint8_t*)&id);
}

void setup(){
  wake_up_instant = millis();
  Serial.begin(115200);
  Serial.print("Cycle: ");
  Serial.println(cycle_count);

  initializeEspNow();
}

void loop(){
  if (!canOperate){
    if (millis() - wake_up_instant > IM_HERE_TIMEOUT_MS){
      Serial.println("Sending IM_HERE message.");
      sendImHereMessage();
      wake_up_instant = millis(); // Reset the timer after sending
    }
  }
  else {
    if (cycle_count % SYNC_PERIOD == 0 && cycle_count != 0){
      sendSyncMessage();
    }

    if (cycle_count % SENSOR_DATA_PERIOD == 0) {
      Serial.println("Reading and sending temperature and humidity values...");
      dht.begin();
      sendSensorData();
    }

    cycle_count++;

    // Ensure the slave is awake for at least 'time_awake_ms'
    unsigned long elapsed_time = millis() - wake_up_instant;
    while (elapsed_time < time_awake_ms){
      // Resend the data message if we haven't received ACK at the timeout expiration
      if (!dataAck && (millis() - timer_start) > ACK_TIMEOUT_MS){
        Serial.println("Timeout expired, resending sensor data message.");
        sendSensorData();
      }
      delay(50);
      elapsed_time = millis() - wake_up_instant;
    }

    if(!dataAck){
      Serial.println("Warning: Data sensor ACK not received at the end of communication window.");
    }

    // Calculate sleep duration
    unsigned long sleep_duration = wake_up_period_ms - (millis() - wake_up_instant) - LEEWAY_MS;
    if (sleep_duration < 0) sleep_duration = 0; // Error handling

    Serial.print("Sleeping for: ");
    Serial.print(sleep_duration / 1000);
    Serial.println(" seconds.");

    // Configure next wake up
    esp_sleep_enable_timer_wakeup(sleep_duration * MS_TO_US);

    Serial.println("Entering Deep Sleep");
    esp_deep_sleep_start();
  }
}
