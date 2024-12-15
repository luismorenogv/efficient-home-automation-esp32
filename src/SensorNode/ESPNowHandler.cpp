/**
 * @file ESPNowHandler.cpp
 * @brief Implementation of ESPNowHandler class for ESP-NOW communication with MasterDevice
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
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
    ackSemaphore = xSemaphoreCreateBinary(); // Used to wait for ACKs
}

bool ESPNowHandler::initializeESPNOW(const uint8_t* master_mac_address, const uint8_t channel) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize ESP-NOW via base class
    return CommunicationsBase::initializeESPNOW();
}

void ESPNowHandler::sendMsg(const uint8_t* data, size_t size) {
    // Assume only one registered peer (master)
    if (numPeers > 0) {
        CommunicationsBase::sendMsg(peers[0].mac_addr, data, size);
    } else {
        LOG_WARNING("No peers registered.");
    }
}

bool ESPNowHandler::waitForAck(MessageType expected_ack, unsigned long timeout_ms) {
    ack_received = false;
    last_acked_msg = expected_ack;

    return (xSemaphoreTake(ackSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) && ack_received;
}

void ESPNowHandler::onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (len < 1) {
        LOG_INFO("Received empty message.");
        return;
    }

    MessageType msg_type = static_cast<MessageType>(data[0]);

    if (msg_type == MessageType::ACK) {
        // Check length and acked_msg match
        if (len >= sizeof(AckMsg)) {
            const AckMsg* ack = reinterpret_cast<const AckMsg*>(data);
            if (ack->acked_msg == last_acked_msg) {
                ack_received = true;
                xSemaphoreGive(ackSemaphore);
                LOG_INFO("ACK received from master");
            }
        } else {
            LOG_WARNING("ACK received with incorrect length.");
        }
    } else if (msg_type == MessageType::NEW_SLEEP_PERIOD) {
        // Interpret as ACK and update sleep period
        if (len >= sizeof(NewSleepPeriodMsg)) {
            const NewSleepPeriodMsg* new_sleep = reinterpret_cast<const NewSleepPeriodMsg*>(data);
            powerManager.updateSleepPeriod(new_sleep->new_period_ms);
            LOG_INFO("NEW_SLEEP_PERIOD interpreted as ACK");
            ack_received = true;

            // Send ACK back to master
            CommunicationsBase::sendAck(mac_addr, MessageType::NEW_SLEEP_PERIOD);

            // Signal semaphore after sending ack
            xSemaphoreGive(ackSemaphore);
        } else {
            LOG_WARNING("NEW_SLEEP_PERIOD message received with incorrect length.");
        }
    } else {
        LOG_WARNING("Received unknown or unhandled message type.");
    }
}