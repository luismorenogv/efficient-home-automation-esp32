/**
 * @file SensorNode.cpp
 * @brief Implementation of SensorNode class for handling sensor readings and ESP-NOW communication
 *
 * @author Luis Moreno
 * @date Nov 22, 2024
 */

#include "SensorConfig.h"
#include "SensorNode/SensorNode.h"
#include "common.h"
#include "mac_addrs.h"



SensorNode::SensorNode(uint8_t room_id)
    : room_id(room_id),
      dhtSensor(DHT_PIN, DHT_TYPE),
      espNowHandler(),
      powerManager(WAKE_INTERVAL_MS)
{
}

void SensorNode::initialize() {
    Serial.begin(115200);
    dhtSensor.initialize();
    if (!espNowHandler.initializeESPNOW(esp32s3_mac)){  // master_mac_address from mac_addrs.h
        powerManager.enterPermanentDeepSleep();
    }
}

void SensorNode::run() {
    float temperature, humidity;
    if (!dhtSensor.readSensorData(temperature, humidity)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.printf("Read sensor data: Temperature = %.2f °C, Humidity = %.2f %%\r\n", temperature, humidity);

        // Prepare data message
        TempHumidMsg msg;
        msg.type = MessageType::TEMP_HUMID;
        msg.room_id = room_id;
        msg.temperature = temperature;
        msg.humidity = humidity;

        // Send data with retries
        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            espNowHandler.sendData(msg);
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
