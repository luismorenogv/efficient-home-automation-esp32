/**
 * @file SensorNode.cpp
 * @brief Implementation of SensorNode class for handling sensor readings and ESP-NOW communication
 *
 * @author Luis Moreno
 * @date Dec 3, 2024
 */

#include "SensorNode/SensorNode.h"

SensorNode::SensorNode(const uint8_t room_id, uint32_t* sleep_duration, uint8_t* channel_wifi, bool* first_cycle)
    : room_id(room_id),
      channel_wifi(channel_wifi),
      first_cycle(first_cycle),
      sht31Sensor(SHT31_ADDRESS, SDA_PIN, SCL_PIN),
      powerManager(sleep_duration),
      espNowHandler(powerManager)
{
}

bool SensorNode::initialize() {
    Serial.begin(115200);
    if (!sht31Sensor.initialize()) {
        Serial.println("SHT31 Sensor initialization failed");
        return false;
    }
    return espNowHandler.initializeESPNOW(master_mac_addr, *channel_wifi);
}

bool SensorNode::joinNetwork(){
    uint8_t channel = *channel_wifi;
    JoinNetworkMsg msg;
    msg.room_id = room_id;
    msg.sleep_period_ms = powerManager.getSleepPeriod();

    for (uint8_t i = 0; i < MAX_WIFI_CHANNEL; i++){
        channel = (channel + i) % MAX_WIFI_CHANNEL;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        espNowHandler.registerPeer((uint8_t*)master_mac_addr, channel);
        // Send data with retries
        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (espNowHandler.waitForAck(MessageType::JOIN_NETWORK, ACK_TIMEOUT_MS)) {
                ack_received = true;
            } else {
                retries++;
                Serial.printf("No ACK received, retrying (%u/%u)\r\n", retries, MAX_RETRIES);
            }
        }

        if (ack_received) {
            Serial.printf("Master found in channel %u", channel);
            *channel_wifi = channel;
            return true;
        }
        Serial.printf("Failed to receive ACK after maximum retries in channel %u\r\n", channel);
        espNowHandler.unregisterPeer((uint8_t*)master_mac_addr);
    }
    return false;
}
    

void SensorNode::run() {
    float temperature, humidity;
    if (!sht31Sensor.readSensorData(temperature, humidity)) {
        Serial.println("Failed to read from SHT31 sensor!");
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
            espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (espNowHandler.waitForAck(MessageType::TEMP_HUMID, ACK_TIMEOUT_MS)) {
                ack_received = true;
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
