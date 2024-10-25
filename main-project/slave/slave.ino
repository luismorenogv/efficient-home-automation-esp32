#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "DHT.h"

// DHT22 Sensor Configuration
#define DHTPIN 4          // GPIO pin connected to DHT22
#define DHTTYPE DHT22     // DHT22 sensor type

#define SLEEP_DURATION 60

DHT dht(DHTPIN, DHTTYPE);

// Structure to hold sensor data
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

struct_message dataToSend;

// Master device MAC address
uint8_t masterAddress[] = {0x3C, 0x84, 0x27, 0xE1, 0xB2, 0xCC};

// Flag to indicate if data was sent
volatile bool dataSent = false;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Packet sent to: ");
  Serial.print(macStr);
  Serial.print(" - Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failure");

  dataSent = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Slave Setup");

  // Initialize DHT22 sensor
  dht.begin();

  // Initialize Wi-Fi in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100); // Ensure Wi-Fi is initialized

  // Set Wi-Fi channel to 1
  if (esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("Failed to set Wi-Fi channel");
  } else {
    Serial.println("Wi-Fi channel set to 1");
  }

  Serial.print("Slave MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }
  Serial.println("ESP-NOW Initialized");

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Configure peer (master)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, masterAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Peer Added Successfully");

  // Configure deep sleep wakeup timer
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * 1000000);
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT22");
  } else {
    // Prepare data
    dataToSend.temperature = temperature;
    dataToSend.humidity = humidity;

    dataSent = false;

    // Send data to master
    esp_err_t result = esp_now_send(masterAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));

    if (result == ESP_OK) {
      Serial.println("Data sent successfully");
    } else {
      Serial.println("Error sending data");
    }

    // Wait for send confirmation or timeout after 5 seconds
    unsigned long start = millis();
    while (!dataSent && millis() - start < 5000) {
      delay(100);
    }

    if (!dataSent) {
      Serial.println("Send timeout");
    }
  }

  // Enter deep sleep
  Serial.println("Entering Deep Sleep for 1 minute");
  esp_deep_sleep_start();
}

