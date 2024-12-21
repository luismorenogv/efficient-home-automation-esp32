/**
 * @file LD2410.cpp
 * @brief Implementation of LD2410 class of RoomNode
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "RoomNode/LD2410.h"

LD2410* LD2410::instance = nullptr;

#define LD2410_BAUDRATE 256000
#define LD2410_SERIAL Serial2

LD2410::LD2410() {
    instance = this;
    pinMode(LD2410_PIN, INPUT);
}

LD2410::~LD2410() {
    detachInterrupt(digitalPinToInterrupt(LD2410_PIN));
    instance = nullptr;
}

// Sets the queue for presence states
void LD2410::setQueue(QueueHandle_t queue) {
    presenceQueue = queue;
}

// Starts the interrupt-based presence detection
void LD2410::start() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t presenceState = digitalRead(LD2410_PIN);
    LOG_INFO("Presence sensor initial state: %s", presenceState == HIGH ? "PRESENCE" : "NO PRESENCE" );
    xQueueSendFromISR(presenceQueue, &presenceState, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
    attachInterrupt(digitalPinToInterrupt(LD2410_PIN), staticPresenceISR, CHANGE);
}

void IRAM_ATTR LD2410::staticPresenceISR() {
    if (instance) {
        instance->presenceISR();
    }
}

// ISR to send presence state to a queue for processing
void IRAM_ATTR LD2410::presenceISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t presenceState = digitalRead(LD2410_PIN);
    xQueueSendFromISR(presenceQueue, &presenceState, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Initializes and configures the LD2410 sensor via UART commands
bool LD2410::initialize() {
    LD2410_SERIAL.begin(LD2410_BAUDRATE, SERIAL_8N1);

    for (uint8_t i = 0; i < MAX_INIT_RETRIES; i++){
        // Enable config mode
        const uint8_t enableConfigCmd[] = {
            0xFD,0xFC,0xFB,0xFA, 0x04,0x00, 0xFF,0x00,0x01,0x00,0x04,0x03,0x02,0x01
        };
        if (!sendCommand(enableConfigCmd, sizeof(enableConfigCmd))) return false;
        if (!waitForAck(0x01FF)) continue;

        // Set no-one duration
        const uint8_t setNoOneCmd[] = {
            0xFD,0xFC,0xFB,0xFA,
            0x14,0x00,
            0x60,0x00,
            0x00,0x00,
            MAXIMUM_MOVING_DISTANCE_GATE,0x00,0x00,0x00,
            0x01,0x00,
            MAXIMUM_STILL_DISTANCE_GATE,0x00,0x00,0x00,
            0x02,0x00,
            UNMANNED_DURATION_S,0x00,0x00,0x00,
            0x04,0x03,0x02,0x01
        };
        if (!sendCommand(setNoOneCmd, sizeof(setNoOneCmd))) return false;
        if (!waitForAck(0x0160)) continue;

        // Set sensitivity
        const uint8_t setSensitivityCmd[] = {
            0xFD,0xFC,0xFB,0xFA,
            0x14,0x00,
            0x64,0x00,
            0x00,0x00,
            0xFF,0xFF,0x00,0x00,
            0x01,0x00,
            SENSITIVITY,0x00,0x00,0x00,
            0x02,0x00,
            SENSITIVITY,0x00,0x00,0x00,
            0x04,0x03,0x02,0x01
        };
        if (!sendCommand(setSensitivityCmd, sizeof(setSensitivityCmd))) return false;
        if (!waitForAck(0x0164)) continue;

        // End config mode
        const uint8_t endConfigCmd[] = {
            0xFD,0xFC,0xFB,0xFA,0x02,0x00,0xFE,0x00,
            0x04,0x03,0x02,0x01
        };
        if (!sendCommand(endConfigCmd, sizeof(endConfigCmd))) return false;
        if (!waitForAck(0x01FE)) continue;

        LOG_INFO("LD2410 successfully configured.");
        return true;
    }

    LOG_ERROR("Unable to initialize LD2410 configuration");
    return false;
}

// Sends a command to LD2410 via UART
bool LD2410::sendCommand(const uint8_t* cmd, size_t len) {
    LD2410_SERIAL.flush();
    LD2410_SERIAL.write(cmd, len);
    LD2410_SERIAL.flush(); 
    return true;
}

// Waits for ACK from LD2410 for a given command word and status
bool LD2410::waitForAck(uint16_t expectedCmdWord, uint8_t expectedStatus) {
    uint8_t buffer[64];
    uint32_t start = millis();
    while((millis() - start) < 500) {
        if(LD2410_SERIAL.available() > 0) {
            size_t len = LD2410_SERIAL.readBytes(buffer, sizeof(buffer));
            if(len >= 8) {
                // Check frame header and parse ACK
                if(buffer[0] == 0xFD && buffer[1] == 0xFC && buffer[2] == 0xFB && buffer[3] == 0xFA) {
                    uint16_t dataLen = buffer[4] | (buffer[5] << 8);
                    if(len >= (6 + dataLen)) {
                        uint8_t cc = buffer[6];
                        uint8_t ce = buffer[7];
                        uint16_t cmdEcho = (ce << 8) | cc;
                        uint8_t status = buffer[8];

                        if(cmdEcho == expectedCmdWord && status == expectedStatus) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    LOG_WARNING("ACK wait timed out or invalid ACK.");
    return false;
}

bool LD2410::getPresence(){
    if (digitalRead(LD2410_PIN) == HIGH){
        return true;
    } else {
        return false;
    }
}