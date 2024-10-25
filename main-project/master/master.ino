#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Structure to receive sensor data
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

struct_message receivedData;

// Data storage configuration
#define DATA_ARRAY_SIZE 144
float temperatureData[DATA_ARRAY_SIZE];
float humidityData[DATA_ARRAY_SIZE];
int dataIndex = 0;

// Callback when data is received
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(struct_message)) {
    Serial.println("Received data size mismatch");
    return;
  }

  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // Print sender's MAC address
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  Serial.print("Data received from: ");
  Serial.println(macStr);

  // Print received data
  Serial.print("Temperature: ");
  Serial.print(receivedData.temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(receivedData.humidity);
  Serial.println(" %");

  // Store data in arrays
  temperatureData[dataIndex] = receivedData.temperature;
  humidityData[dataIndex] = receivedData.humidity;

  // Update index
  dataIndex = (dataIndex + 1) % DATA_ARRAY_SIZE;
  Serial.print("Data index updated to: ");
  Serial.println(dataIndex);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Master Setup");

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

  Serial.print("Master MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }
  Serial.println("ESP-NOW Initialized");

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // No actions needed in loop
}
