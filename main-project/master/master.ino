#include "master.h"
#include <WiFi.h>

// ----------------------------
// Global Variables
// ----------------------------

// Data structure to hold received sensor data
struct_message received_data;

// Arrays to store temperature and humidity data
float temperature_data[DATA_ARRAY_SIZE];
float humidity_data[DATA_ARRAY_SIZE];
int data_index = 0;  // Index to track data storage location

// ----------------------------
// Callback Function
// ----------------------------

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incoming_data, int len) {
    if (len != sizeof(struct_message)) {
        Serial.println("Received data size mismatch");
        return;
    }

    // Copy received data into the struct
    memcpy(&received_data, incoming_data, sizeof(received_data));

    // Print sender's MAC address
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.print("Data received from: ");
    Serial.println(mac_str);

    // Print sensor data
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
}

void haltExecution() {
    Serial.println("Execution halted");
    while (true) {
        delay(1000);  // Halt execution on failure
    }
}
// ----------------------------
// Setup Function
// ----------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("Master Setup");

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Initialization Failed");
        haltExecution();
    }
    Serial.println("ESP-NOW Initialized");

    // Register receive callback
    esp_now_register_recv_cb(onDataRecv);
}

// ----------------------------
// Loop Function
// ----------------------------

void loop() {
    // No actions needed in loop
}