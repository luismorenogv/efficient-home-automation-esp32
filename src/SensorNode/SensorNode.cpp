/**
 * @file SensorNode.cpp
 * @brief Implementation of SensorNode class for handling sensor readings and ESP-NOW communication
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "SensorNode/SensorNode.h"

SensorNode::SensorNode(const uint8_t room_id, uint32_t* sleep_duration, uint8_t* channel_wifi, bool* first_cycle)
    : room_id(room_id),
      channel_wifi(channel_wifi),
      first_cycle(first_cycle),
      sht31Sensor(SHT31_ADDRESS, SDA_PIN, SCL_PIN),
      powerManager(sleep_duration),
      espNowHandler(powerManager) {}

bool SensorNode::initialize() {
    Serial.begin(115200);
    if (!sht31Sensor.initialize()) {
        LOG_ERROR("SHT31 initialization failed");
        return false;
    }
    // Initialize ESP-NOW; if not first cycle, peer registration occurs after join
    return espNowHandler.initializeESPNOW(master_mac_addr, *channel_wifi);
}

bool SensorNode::joinNetwork() {
    uint8_t channel = *channel_wifi;
    JoinSensorMsg msg;
    msg.room_id = room_id;
    msg.sleep_period_ms = powerManager.getSleepPeriod();

    // Cycle through channels to find the Master
    for (uint8_t i = 0; i < MAX_WIFI_CHANNEL; i++) {
        channel = (channel + i) % MAX_WIFI_CHANNEL;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

        espNowHandler.registerPeer((uint8_t*)master_mac_addr, channel);
        bool ack_received = false;
        uint8_t retries = 0;

        while (!ack_received && retries < MAX_RETRIES) {
            espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (espNowHandler.waitForAck(MessageType::JOIN_SENSOR, ACK_TIMEOUT_MS)) {
                ack_received = true;
            } else {
                retries++;
                LOG_WARNING("No ACK, retrying (%u/%u)", retries, MAX_RETRIES);
            }
        }

        if (ack_received) {
            LOG_INFO("Master found on channel %u", channel);
            *channel_wifi = channel;
            return true;
        }
        LOG_INFO("No ACK on channel %u, trying next.", channel);
        espNowHandler.unregisterPeer((uint8_t*)master_mac_addr);
    }
    return false;
}

void SensorNode::run() {
    float temperature, humidity;

    espNowHandler.registerPeer((uint8_t*)master_mac_addr, *channel_wifi);

    if (!sht31Sensor.readSensorData(temperature, humidity)) {
        LOG_WARNING("Failed to read SHT31!");
    } else {
        LOG_INFO("Sensor data: Temp=%.2f°C, Hum=%.2f%%", temperature, humidity);
        TempHumidMsg msg;
        msg.room_id = room_id;
        msg.temperature = temperature;
        msg.humidity = humidity;

        bool ack_received = false;
        uint8_t retries = 0;
        while (!ack_received && retries < MAX_RETRIES) {
            espNowHandler.sendMsg(reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
            if (espNowHandler.waitForAck(MessageType::TEMP_HUMID, ACK_TIMEOUT_MS)) {
                ack_received = true;
            } else {
                retries++;
                LOG_WARNING("No ACK for TEMP_HUMID, retry (%u/%u)", retries, MAX_RETRIES);
            }
        }

        if (!ack_received) {
            LOG_ERROR("No ACK after max retries for sensor data");
        }
    }
}

void SensorNode::goSleep() {
    powerManager.enterDeepSleep();
}