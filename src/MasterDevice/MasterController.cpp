/**
 * @file MasterController.cpp
 * @brief Implementation of MasterController class to coordinate all modules
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#include "MasterDevice/MasterController.h"


static MasterController* masterControllerInstance = nullptr;

MasterController::MasterController() 
    : communications(), 
      webSockets(dataManager) 
{
    masterControllerInstance = this;
    espnowQueue = xQueueCreate(10, sizeof(IncomingMsg));
}

void MasterController::initialize() {
    Serial.begin(115200);
    delay(1000);

    communications.initializeWifi();
    if (!communications.initializeESPNOW()){
        esp_deep_sleep_start();
    }
    communications.setQueue(espnowQueue);
    ntpClient.initialize();
    webServer.initialize();

    webSockets.initialize(webServer.getServer());
    webSockets.setSleepDurationCallback(MasterController::sleepPeriodChangedCallback);

    BaseType_t result;

    // ESP-NOW Task
    result = xTaskCreatePinnedToCore(
        espnowTask,
        "ESP-NOW Task",
        8192,
        this,
        2,
        &espnowTaskHandle,
        1
    );
    if (result != pdPASS) {
        Serial.println("Failed to create ESP-NOW Task");
    }

    // Web Server Task
    result = xTaskCreatePinnedToCore(
        webServerTask,
        "Web Server Task",
        8192,
        this,
        1,
        &webServerTaskHandle,
        0
    );
    if (result != pdPASS) {
        Serial.println("Failed to create Web Server Task");
    }
}

void MasterController::sleepPeriodChangedCallback(uint8_t room_id, uint32_t new_sleep_period_ms) {
    if (masterControllerInstance) {
        Serial.printf("New sleep period petition received by Master for room %u: %u ms\r\n", room_id, new_sleep_period_ms);
        masterControllerInstance->dataManager.setNewSleepPeriod(room_id, new_sleep_period_ms);
    }
}

void MasterController::espnowTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);
    IncomingMsg msg;

    while (true) {
        if (xQueueReceive(self->espnowQueue, &msg, portMAX_DELAY) == pdTRUE) {
            MessageType msg_type = static_cast<MessageType>(msg.data[0]);
            switch (msg_type) {
                case MessageType::TEMP_HUMID:
                    {
                        TempHumidMsg* payload = reinterpret_cast<TempHumidMsg*>(msg.data);
                        uint8_t room_id = payload->room_id;
                        float temperature = payload->temperature;
                        float humidity = payload->humidity;
                        time_t timestamp = time(nullptr);
                        self->dataManager.addSensorData(room_id, temperature, humidity, timestamp);

                        // Send update via WebSockets
                        self->webSockets.sendDataUpdate(room_id);

                        // Check if pending_update is true
                        if (self->dataManager.isPendingUpdate(room_id)) {
                            uint32_t new_period = self->dataManager.getNewSleepPeriod(room_id);

                            NewSleepPeriodMsg new_period_msg;
                            new_period_msg.type = MessageType::NEW_SLEEP_PERIOD;
                            new_period_msg.new_period_ms = new_period;

                            uint8_t sensor_mac[MAC_ADDRESS_LENGTH];
                            if (self->dataManager.getMacAddr(room_id, sensor_mac)) {
                                self->communications.sendMsg(sensor_mac, reinterpret_cast<uint8_t*>(&new_period_msg), sizeof(NewSleepPeriodMsg));
                                Serial.printf("Sent NEW_SLEEP_PERIOD to sensor in room %u successfully\r\n", room_id);
                            } else {
                                Serial.printf("Failed to retrieve MAC address for room %u. Cannot send NEW_SLEEP_PERIOD.\r\n", room_id);
                            }

                            // Update sleep_period_ms and reset the flag
                            self->dataManager.sleepPeriodWasUpdated(room_id);
                        } else {
                            self->communications.sendAck(msg.mac_addr, MessageType::TEMP_HUMID);
                        }
                    }
                    break;
                case MessageType::JOIN_NETWORK:
                    {
                        JoinNetworkMsg* payload = reinterpret_cast<JoinNetworkMsg*>(msg.data);
                        uint8_t room_id = payload->room_id;
                        uint32_t sleep_period_ms = payload->sleep_period_ms;
                        self->dataManager.sensorSetup(room_id, msg.mac_addr, sleep_period_ms);

                        Serial.printf("Received JOIN_NETWORK from room %u with sleep_period %u ms\r\n", room_id, sleep_period_ms);
                        self->communications.registerPeer(msg.mac_addr);
                        self->communications.sendAck(msg.mac_addr, msg_type);
                    }
                    break;
                case MessageType::ACK:
                    {
                        if (msg.len < sizeof(AckMsg)) {
                            Serial.println("ACK message too short.");
                            break;
                        }
                        AckMsg* payload = reinterpret_cast<AckMsg*>(msg.data);
                        if (payload->acked_msg == MessageType::NEW_SLEEP_PERIOD){
                            uint8_t room_id = self->dataManager.getId(msg.mac_addr);
                            if (room_id != ID_NOT_VALID){
                                self->dataManager.sleepPeriodWasUpdated(room_id);
                            } else {
                                Serial.println("Received ACK for NEW_SLEEP_PERIOD from unknown SensorNode.");
                            }
                        } else {
                            Serial.printf("Received ACK for unknown MessageType: %d\r\n", payload->acked_msg);
                        }
                    }
                    break;
                default:
                    Serial.printf("Received unknown message type: %d\r\n", msg_type);
            }
        }
    }
}

void MasterController::webServerTask(void* pvParameter) {
    MasterController* self = static_cast<MasterController*>(pvParameter);

    self->webServer.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
