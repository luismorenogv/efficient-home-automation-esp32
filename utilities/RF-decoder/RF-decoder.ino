#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Connection Pins
#define CC1101_GDO0_PIN 4   // Interrupt pin (GDO0)
#define CC1101_SS_PIN 5     // CSN/SS pin
#define LED_DEBUGGING 2     // Debug LED pin

int led_state = 0;

// Structure to store CC1101 configurations
struct CC1101Config {
  int modulation;          // 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK
  float frequency;         // MHz
  float deviation;         // kHz
  float dataRate;          // kBaud
  float rxBW;              // kHz
  bool crcEnabled;         // CRC enabled/disabled
  uint8_t syncWordHigh;    // Sync Word High byte
  uint8_t syncWordLow;     // Sync Word Low byte
};

byte buffer[61] = {0};

// Define 7 configurations
const int NUM_CONFIGS = 7;
CC1101Config configs[NUM_CONFIGS] = {
  {2, 433.92, 20.0, 1.2, 58.03, false, 0xD3, 0x91},
  {2, 433.92, 30.0, 4.8, 100.0, false, 0xD3, 0x91},
  {2, 433.92, 47.60, 9.6, 200.0, true, 0xD3, 0x91},
  {0, 433.92, 10.0, 1.2, 58.03, true, 0xD3, 0x91},
  {0, 433.92, 20.0, 2.4, 100.0, true, 0xD3, 0x91},
  {1, 433.92, 20.0, 38.4, 58.03, true, 0xD3, 0x91},
  {4, 433.92, 47.60, 99.97, 812.50, true, 0xD3, 0x91}
};

int currentConfigIndex = 0; // Current configuration index

// Function to configure CC1101 with specific parameters
void configureCC1101(const CC1101Config &config) {
  Serial.println("Configuring CC1101 with new settings...");

  // Set SPI pins: SCK, MISO, MOSI, SS
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, CC1101_SS_PIN);

  // Set interrupt pin GDO0
  ELECHOUSE_cc1101.setGDO0(CC1101_GDO0_PIN);
  
  ELECHOUSE_cc1101.Init();
  Serial.println("CC1101 Initialized.");

  // Set frequency
  ELECHOUSE_cc1101.setMHZ(config.frequency);
  Serial.print("Frequency set to ");
  Serial.print(config.frequency);
  Serial.println(" MHz");

  // Set modulation
  ELECHOUSE_cc1101.setModulation(config.modulation);
  Serial.print("Modulation set to ");
  switch(config.modulation){
    case 0: Serial.println("2-FSK"); break;
    case 1: Serial.println("GFSK"); break;
    case 2: Serial.println("ASK/OOK"); break;
    case 3: Serial.println("4-FSK"); break;
    case 4: Serial.println("MSK"); break;
    default: Serial.println("Unknown"); break;
  }

  // Set frequency deviation
  ELECHOUSE_cc1101.setDeviation(config.deviation);
  Serial.print("Frequency Deviation set to ");
  Serial.print(config.deviation);
  Serial.println(" kHz");

  // Set data rate
  ELECHOUSE_cc1101.setDRate(config.dataRate);
  Serial.print("Data Rate set to ");
  Serial.print(config.dataRate);
  Serial.println(" kBaud");

  // Set reception bandwidth
  ELECHOUSE_cc1101.setRxBW(config.rxBW);
  Serial.print("Reception Bandwidth set to ");
  Serial.print(config.rxBW);
  Serial.println(" kHz");

  // Enable or disable CRC
  ELECHOUSE_cc1101.setCrc(config.crcEnabled ? 1 : 0);
  Serial.print("CRC ");
  Serial.println(config.crcEnabled ? "Enabled" : "Disabled");

  // Set Sync Word
  ELECHOUSE_cc1101.setSyncWord(config.syncWordHigh, config.syncWordLow);
  Serial.print("Sync Word set to ");
  Serial.print(config.syncWordHigh, HEX);
  Serial.print(", ");
  Serial.println(config.syncWordLow, HEX);

  // Disable Manchester encoding
  ELECHOUSE_cc1101.setManchester(0);
  Serial.println("Manchester encoding disabled");

  // Set CC1101 to receive mode
  ELECHOUSE_cc1101.SetRx();
  Serial.println("Receive mode activated.");
}

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting setup...");

  // Configure debug LED
  pinMode(LED_DEBUGGING, OUTPUT);

  // Configure CC1101 with the first configuration
  configureCC1101(configs[currentConfigIndex]);
}

void loop(){
  // Check if data is received
  if (ELECHOUSE_cc1101.CheckRxFifo(100)){
    // Check CRC
    if (ELECHOUSE_cc1101.CheckCRC()){ 

      // Toggle debug LED
      led_state = !led_state;
      digitalWrite(LED_DEBUGGING, led_state ? HIGH : LOW);

      // Get RSSI and LQI (Quality of the signal)
      int rssi = ELECHOUSE_cc1101.getRssi();
      byte lqi = ELECHOUSE_cc1101.getLqi();

      Serial.print("RSSI: ");
      Serial.println(rssi);
      Serial.print("LQI: ");
      Serial.println(lqi);

      // Receive data
      int len = ELECHOUSE_cc1101.ReceiveData(buffer);
      buffer[len] = '\0'; // Null-terminate string

      // Print received data as string
      Serial.print("Received (char): ");
      Serial.println((char *) buffer);

      // Print received data as hex bytes
      Serial.print("Received (bytes): ");
      for (int i = 0; i < len; i++){
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("Error detected by CRC");
    }
  }

  // Check for serial input to change configuration
  if (Serial.available() > 0){
    Serial.read(); // Read received character

    // Switch to next configuration
    currentConfigIndex = (currentConfigIndex + 1) % NUM_CONFIGS;

    Serial.print("Switching to configuration ");
    Serial.println(currentConfigIndex + 1);

    // Apply new configuration
    configureCC1101(configs[currentConfigIndex]);
  }
}