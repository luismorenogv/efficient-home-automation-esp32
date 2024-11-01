/**
 * @file slave.ino 
 * @brief Slave ESP32 main code
 *
 * @author Luis Moreno
 * @date October 26, 2024
 */

#include "slave.h"
#include <WiFi.h>
#include "DHT.h"

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Structure for sensor data
SensorData data_to_send;

// Command received from the master
Message received_message;

unsigned long start_time = 0;

// Flags and state variables
volatile bool data_sent = false;
RTC_DATA_ATTR bool canOperate = false; // Flag indicating if the slave can start operating

// RTC memory attributes to retain data during deep sleep
RTC_DATA_ATTR uint8_t devices_state = 0x00;
RTC_DATA_ATTR uint16_t cycle_count = 0;


void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    data_sent = true;
    Serial.println("Message sent successfully.");
  } else {
    Serial.println("Error sending message.");
  }
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < 1) {
      Serial.println("Received empty message.");
      return;
  }

  MessageType msg_type = static_cast<MessageType>(incoming_data[0]);

  switch (msg_type) {
    case START:
      Serial.println("Received START from master.");
      canOperate = true; // Allow operations to begin
      break;

    case SYNC:
      // TODO: synchronize the slave to the master
      break;

    case LIGHTS:
      // TODO
      break;

    case AC:
      // TODO
      break;

    default:
      Serial.println("Received unknown or unhandled message type.");
      break;
  }
}

void initializeEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    haltExecution();
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  // Register the send callback
  esp_now_register_send_cb(onDataSent);

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


void turnLights(bool state) {
  if (state) {
    devices_state |= LIGHTS_MASK;
    // TODO: Activate lights via RF
    Serial.println("Lights ON");
  }
  else {
    devices_state &= ~LIGHTS_MASK;
    // TODO: Deactivate lights via RF
    Serial.println("Lights OFF");
  }
}

void turnAirConditioner(bool state) {
  if (state) {
    devices_state |= AIR_CONDITIONER_MASK;
    // TODO: Activate air conditioner via IR
    Serial.println("Air Conditioner ON");
  }
  else {
    devices_state &= ~AIR_CONDITIONER_MASK;
    // TODO: Deactivate air conditioner via IR
    Serial.println("Air Conditioner OFF");
  }
}

void readAndSendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT22 read failed");
    return;
  }

  data_to_send.temperature = temperature;
  data_to_send.humidity = humidity;

  data_sent = false;

  // Build message SENSOR_DATA
  Message data_message;
  data_message.type = SENSOR_DATA;
  memcpy(data_message.payload, &data_to_send, sizeof(data_to_send));

  esp_err_t result = esp_now_send(MASTER_ADDRESS, (uint8_t *)&data_message, sizeof(data_message.type) + sizeof(data_message.payload));

  if (result != ESP_OK) {
    Serial.println("Error sending SENSOR_DATA message.");
  }

  unsigned long start = millis();
  while (!data_sent && millis() - start < 5000);

  if (!data_sent) {
    Serial.println("Send timeout for SENSOR_DATA message.");
  }
}


void haltExecution(){
  while (true) {
    Serial.print("Execution halted");
    delay(1000);
  }
}

void sendImHereMessage() {
  Message im_here_message;
  im_here_message.type = IM_HERE;

  esp_err_t result = esp_now_send(MASTER_ADDRESS, (uint8_t *)&im_here_message, FRAME_SIZE[IM_HERE]);

  if (result == ESP_OK) {
    Serial.println("IM_HERE message sent successfully.");
  } else {
    Serial.println("Error sending IM_HERE message.");
  }
}


void setup() {
  Serial.begin(115200);
  Serial.print("Cicle: ");
  Serial.println(cycle_count);

  initializeEspNow();
}

void loop() {
  if (!canOperate){
    if (millis() - start_time > IM_HERE_INTERVAL_MS){
      sendImHereMessage();
      start_time = millis();
    }
  }
  else {
    if (cycle_count % SEND_DATA_INTERVAL_S == 0) {
      Serial.println("Reading and sending temperature and humidity values...");

      dht.begin();

      readAndSendSensorData();
    }

    // Poll LD2410 for presence detection if any device is active
    if (cycle_count % LD2410_POLLING_INTERVAL_S == 0 && devices_state != 0) {
      // Activate transistor connected to the power
      pinMode(LD2410_POWER_PIN, OUTPUT);
      digitalWrite(LD2410_POWER_PIN, HIGH);
      Serial.println("LD2410 Activated for presence detection");
      
      // TODO: Algorithm to interpret the sensor (Stabilization, consistency...)


      // Deactivate LD2410
      digitalWrite(LD2410_POWER_PIN, LOW);
    }

    cycle_count++;

    // Configure next wake up
    esp_sleep_enable_timer_wakeup(WAKE_UP_PERIOD_MS * MS_TO_US_FACTOR);

    Serial.println("Entering Deep Sleep");
    esp_deep_sleep_start();
  }
}
