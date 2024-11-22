/**
 * @file SensorConfig.h
 * @brief Configuration constants for SensorNode
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */
#pragma once
#include <Arduino.h>
#include <DHT.h>

// Define Room ID (unique for each SensorNode)
constexpr uint8_t ROOM_ID = 0;

// Communication
constexpr unsigned long WAKE_INTERVAL_MS = 300000; // 5 minutes
constexpr unsigned long ACK_TIMEOUT_MS = 5000;     // 5 seconds
constexpr uint8_t MAX_RETRIES = 3;
constexpr uint8_t MAX_INIT_RETRIES = 3;

// DHT Sensor configuration
constexpr uint8_t DHT_PIN = 4; // GPIO pin connected to DHT22
constexpr uint8_t DHT_TYPE = DHT22;
