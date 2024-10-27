#include "slave.h"
#include <WiFi.h>
#include "DHT.h"

// ----------------------------
// Global Variables
// ----------------------------

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Structure to hold sensor data
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

// Sensor data message
struct_message data_to_send;

// Flags and State Variables
volatile bool data_sent = false;
RTC_DATA_ATTR bool send_climate_data = true;

// RTC memory attributes to retain data during deep sleep
RTC_DATA_ATTR uint8_t devices_state = 0x00;

// ----------------------------
// Callback Function
// ----------------------------

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Packet sent to: ");
  Serial.print(mac_str);
  Serial.print(" - Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failure");

  data_sent = true;
}

// ----------------------------
// Function Definitions
// ----------------------------

void initializeEspNow() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    haltExecution();
  }
  Serial.println("ESP-NOW Initialized");

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Configure peer (master)
  esp_now_peer_info_t peer_info;
  memcpy(peer_info.peer_addr, MASTER_ADDRESS, 6);
  peer_info.channel = 0;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add peer");
    haltExecution();
  }
  Serial.println("Peer Added Successfully");
}

void turnLights(bool state) {
  if (state) {
    devices_state |= LIGHTS_MASK;
    // TODO: Activate lights through RF 
    // TODO: Activate PIR Sensor 
  }
  else {
    devices_state &= ~LIGHTS_MASK;
    // TODO: Deactivate lights through RF 
  }
}

void turnAirConditioner(bool state) {
  if (state) {
    devices_state |= AIR_CONDITIONER_MASK;
    // TODO: Activate Air Conditioner through IR 
    // TODO: Activate PIR Sensor
  }
  else {
    devices_state &= ~AIR_CONDITIONER_MASK;
    // TODO: Deactivate Air Conditioner through IR 
  }
}

void readAndSendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Ensure valid data
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT22");
  }
  else {
    // Prepare data struct
    data_to_send.temperature = temperature;
    data_to_send.humidity = humidity;

    // Turn off flag before sending
    data_sent = false;

    // Send data to master
    esp_err_t result = esp_now_send(MASTER_ADDRESS, (uint8_t *)&data_to_send, sizeof(data_to_send));

    if (result == ESP_OK) {
      Serial.println("Data sent successfully");
    }
    else {
      Serial.println("Error sending data");
    }

    // Wait for send confirmation or timeout after 5 seconds
    unsigned long start = millis();
    while (!data_sent && millis() - start < 5000);

    if (!data_sent) {
      Serial.println("Send timeout");
    }
  }
}

void haltExecution(){
  Serial.println("Execution halted");
  while (true) {
    delay(1000); // Halt execution on failure
  }
}

// ----------------------------
// Setup Function
// ----------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("Slave Setup");

  // Determine wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      // The room has been empty for a certain amount of time configured by hardware
      Serial.println("Wakeup caused by PIR");  
      if ((devices_state & LIGHTS_MASK) != 0) {
        turnLights(false);
      }
      if ((devices_state & AIR_CONDITIONER_MASK) != 0) {
        turnAirConditioner(false);
      }
      // TODO: Turn off PIR Sensor
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      send_climate_data = true;
      break;
    default:
      Serial.printf("Wakeup was caused by: %d\n", wakeup_reason);
      break;
  }

  if (send_climate_data) {
    // Initialize DHT22 sensor
    dht.begin();

    // Initialize ESP-NOW
    initializeEspNow();

    // Read and send sensor data
    readAndSendSensorData();

    send_climate_data = false;
  }

  // Configure deep sleep wakeup timer
  esp_sleep_enable_timer_wakeup(SENDING_PERIOD * 1000000); // 60 seconds in microseconds

  if (devices_state != 0) {
    // Enable PIR low level interrupt if light and/or AC are on
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 0); // Wake up when PIR_PIN is LOW
    // TODO: Turn on PIR Sensor
  }

  // Enter deep sleep
  Serial.println("Entering Deep Sleep for 1 minute");
  esp_deep_sleep_start();
}

// ----------------------------
// Loop Function
// ----------------------------

void loop() {
  // Not used
}
