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
struct_message data_to_send;

// Command recieved from the master
uint8_t received_command;

// Flags and state variables
volatile bool data_sent = false;

// RTC memory attributes to retain data during deep sleep
RTC_DATA_ATTR uint8_t devices_state = 0x00;
RTC_DATA_ATTR uint16_t cycle_count = 0;


void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  data_sent = true;
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
  if (len < sizeof(received_command)) return;

  memcpy(&received_command, incoming_data, sizeof(received_command));

  // Manage command (sync clock or configure lights or air conditioner)
}

void initializeEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    haltExecution();
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);
}

void configureEspNowSending() {
  // Registrar la callback de envío
  esp_now_register_send_cb(onDataSent);

  // Configurar peer (master)
  esp_now_peer_info_t peer_info;
  memset(&peer_info, 0, sizeof(peer_info)); // Inicializar todos los campos a 0
  memcpy(peer_info.peer_addr, MASTER_ADDRESS, 6);
  peer_info.channel = WiFi.channel();  // Usa el canal actual
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add peer");
    haltExecution();
  }
}


// Control lights
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

// Control air conditioner
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

// Read sensor data and send
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

  esp_err_t result = esp_now_send(MASTER_ADDRESS, (uint8_t *)&data_to_send, sizeof(data_to_send));

  if (result != ESP_OK) {
    Serial.println("Error sending data");
  }

  unsigned long start = millis();
  while (!data_sent && millis() - start < 5000);

  if (!data_sent) {
    Serial.println("Send timeout");
  }
}

// Halt execution
void haltExecution(){
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Slave Setup");

  dht.begin();

  initializeEspNow();

  if (cycle_count % SEND_DATA_INTERVAL_S == 0) {
    Serial.println("Reading and sending temperature and humidity values...");
    configureEspNowSending();

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

  // Increment cycle count
  cycle_count++;

  // Configure next wake up
  esp_sleep_enable_timer_wakeup(WAKE_UP_PERIOD_MS * MS_TO_US_FACTOR);

  // Enter deep sleep
  Serial.println("Entering Deep Sleep");
  esp_deep_sleep_start();
}

void loop() {
  // Not used
}

