/**
 * @file SensorNode.cpp
 * @brief Implementation of SensorNode class for handling sensor readings and ESP-NOW communication
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "config.h"
#include "SensorNode/SensorNode.h"
#include "common.h"
#include "mac_addrs.h"



SensorNode::SensorNode(const uint8_t room_id, uint32_t* sleep_duration)
    : room_id(room_id),
      dhtSensor(DHT_PIN, DHT_TYPE),
      powerManager(sleep_duration),
      espNowHandler(powerManager)
{
}

bool SensorNode::initialize(uint8_t wifi_channel) {
    Serial.begin(115200);
    dhtSensor.initialize();
    return espNowHandler.initializeESPNOW(master_mac_addr, wifi_channel);
    
}

bool SensorNode::joinNetwork(){
    JoinNetworkMsg msg;
    msg.room_id = room_id;
    msg.sleep_period_ms = powerManager.getSleepPeriod();
    
    // Send data with retries
    bool ack_received = false;
    uint8_t retries = 0;
    while (!ack_received && retries < MAX_RETRIES) {
        espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg)); // Corrección
        if (espNowHandler.waitForAck(MessageType::JOIN_NETWORK, ACK_TIMEOUT_MS)) {
            ack_received = true;
        } else {
            retries++;
            Serial.printf("No ACK received, retrying (%u/%u)\r\n", retries, MAX_RETRIES);
        }
    }

    if (!ack_received) {
        Serial.println("Failed to receive ACK after maximum retries");
        return false;
    }
    return true;
}


void SensorNode::run() {
    float temperature, humidity;
    if (!dhtSensor.readSensorData(temperature, humidity)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.printf("Read sensor data: Temperature = %.2f °C, Humidity = %.2f %%\r\n", temperature, humidity);

        // Prepare data message
        TempHumidMsg msg;
        msg.room_id = room_id;
        msg.temperature = temperature;
        msg.humidity = humidity;

        // Send data with retries
        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg)); // Corrección
            if (espNowHandler.waitForAck(MessageType::TEMP_HUMID, ACK_TIMEOUT_MS)) {
                ack_received = true;
                Serial.println("ACK received");
            } else {
                retries++;
                Serial.printf("No ACK received, retrying (%u/%u)\r\n", retries, MAX_RETRIES);
            }
        }

        if (!ack_received) {
            Serial.println("Failed to receive ACK after maximum retries");
        }
    }
}


void SensorNode::goSleep(){
    // Enter deep sleep
    powerManager.enterDeepSleep();
}
