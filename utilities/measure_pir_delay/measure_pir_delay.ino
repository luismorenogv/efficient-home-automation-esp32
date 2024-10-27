/**
 * @file measure_pir_delay.ino
 * @brief Measures the delay duration of a PIR sensor from HIGH to LOW.
 *
 * @author Luis Moreno
 * @date October 26, 2024
 */

#include <Arduino.h>

// ----------------------------
// Constants and Pin Definitions
// ----------------------------

#define PIR_PIN 25                // GPIO pin connected to PIR sensor's OUT

// ----------------------------
// Global Variables
// ----------------------------

bool pirState = LOW;               // Current state of PIR sensor
bool lastPirState = LOW;           // Previous state of PIR sensor
unsigned long startTime = 0;       // Time when PIR was triggered (in milliseconds)
unsigned long duration = 0;        // Duration the PIR stays HIGH (in milliseconds)

// ----------------------------
// Setup Function
// ----------------------------

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);

  // Initialize PIR sensor pin as input
  pinMode(PIR_PIN, INPUT);

  // Print initial instructions
  Serial.println("======================================");
  Serial.println("     PIR Sensor Delay Measurement");
  Serial.println("======================================");
}

// ----------------------------
// Loop Function
// ----------------------------

void loop() {
  // Read the current state of the PIR sensor
  pirState = digitalRead(PIR_PIN);/*

  // Detect rising edge
  if (pirState == HIGH && lastPirState == LOW) {
    Serial.println("PIR Triggered: HIGH detected.");
    startTime = millis(); // Start the timer
  }

  // Detect falling edge
  if (pirState == LOW && lastPirState == HIGH) {
    duration = millis() - startTime; // Calculate duration
    float durationSeconds = duration / 1000.0; // Convert to seconds
    Serial.print("PIR Released: LOW detected. Duration: ");
    Serial.print(durationSeconds, 2);
    Serial.println(" seconds.\n");
  }

  // Update the last state
  lastPirState = pirState;*/

  Serial.println(pirState); // NOTE: PIR Module Malfunctioning

  delay(50);

}