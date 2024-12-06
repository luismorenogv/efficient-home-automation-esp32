/**
 * @file ESPNowHandler.cpp
 * @brief Implementation of ESPNowHandler class for ESP-NOW communication with MasterDevice
 * 
 * @author Luis Moreno
 * @date Dec 5, 2024
 */

#include "SensorNode/ESPNowHandler.h"
#include "esp_wifi.h"

ESPNowHandler* ESPNowHandler::instance = nullptr;

ESPNowHandler::ESPNowHandler(PowerManager& powerManager) 
    : CommunicationsBase(),
      powerManager(powerManager),
      ack_received(false),
      last_acked_msg(MessageType::ACK) {
    instance = this;
    ackSemaphore = xSemaphoreCreateBinary();
}


bool ESPNowHandler::initializeESPNOW(const uint8_t* master_mac_address, const uint8_t channel) {
    // Initialize Wi-Fi in Station Mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize ESP-NOW via base class
    if (!CommunicationsBase::initializeESPNOW()) {
        return false;
    }
    return true;
}

void ESPNowHandler::sendMsg(const uint8_t* data, size_t size) {
    // Assume only one peer (master)
    if (numPeers > 0) {
        CommunicationsBase::sendMsg(peers[0].mac_addr, data, size);
    } else {
        Serial.println("No peers registered.");
    }
}

bool ESPNowHandler::waitForAck(MessageType expected_ack, unsigned long timeout_ms) {
    ack_received = false;
    last_acked_msg = expected_ack;

    if (xSemaphoreTake(ackSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ack_received;
    }
    return false;
}

void ESPNowHandler::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (len < 1) {
        Serial.println("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);

    if (msg_type == MessageType::ACK) {
        if (len >= sizeof(AckMsg)) {
            const AckMsg* ack = reinterpret_cast<const AckMsg*>(data);
            if (ack->acked_msg == last_acked_msg) {
                ack_received = true;
                xSemaphoreGive(ackSemaphore); // Signal ACK reception
                Serial.println("ACK received from master");
            }
        } else {
            Serial.println("ACK received with incorrect length.");
        }
    } else if (msg_type == MessageType::NEW_SLEEP_PERIOD) {
        if (len >= sizeof(NewSleepPeriodMsg)) {
            const NewSleepPeriodMsg* new_sleep = reinterpret_cast<const NewSleepPeriodMsg*>(data);
            powerManager.updateSleepPeriod(new_sleep->new_period_ms);
            Serial.println("NEW_SLEEP_PERIOD interpreted as ACK");
            ack_received = true;

            // Send ACK to master
            AckMsg ack_msg;
            ack_msg.type = MessageType::ACK; // Ensure the ACK message type is correctly set
            ack_msg.acked_msg = MessageType::NEW_SLEEP_PERIOD;
            CommunicationsBase::sendMsg((uint8_t*)mac_addr, reinterpret_cast<const uint8_t*>(&ack_msg), sizeof(ack_msg));
        } else {
            Serial.println("NEW_SLEEP_PERIOD message received with incorrect length.");
        }
    } else {
        Serial.println("Received unknown or unhandled message type.");
    }
}
