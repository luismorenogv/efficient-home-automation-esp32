#include "RoomNode/LD2410.h"

// Static instance pointer
LD2410* LD2410::instance = nullptr;

// Example UART configuration: adapt as needed
#define LD2410_BAUDRATE 256000
#define LD2410_SERIAL Serial2

LD2410::LD2410(){
    instance = this;
    pinMode(LD2410_PIN, INPUT);
}

LD2410::~LD2410() {
    detachInterrupt(digitalPinToInterrupt(LD2410_PIN));
    instance = nullptr;
}

void LD2410::setQueue(QueueHandle_t queue){
    presenceQueue = queue;
}

void LD2410::start(){
    attachInterrupt(digitalPinToInterrupt(LD2410_PIN), staticPresenceISR, CHANGE);
}

void IRAM_ATTR LD2410::staticPresenceISR() {
    if (instance) {
        instance->presenceISR();
    }
}

void IRAM_ATTR LD2410::presenceISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t presenceState = digitalRead(LD2410_PIN);
    xQueueSendFromISR(presenceQueue, &presenceState, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

bool LD2410::initialize() {
    // Initialize UART
    LD2410_SERIAL.begin(LD2410_BAUDRATE, SERIAL_8N1);

    // 1) Enable Configuration Mode
    const uint8_t enableConfigCmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00,
        0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01
    };
    if (!sendCommand(enableConfigCmd, sizeof(enableConfigCmd))) return false;
    if (!waitForAck(0x01FF)) return false;

    // 2) Set "No-One Duration" to 30s
    const uint8_t setNoOneCmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA,
        0x14, 0x00,
        0x60, 0x00,
        0x00, 0x00,
        MAXIMUM_MOVING_DISTANCE_GATE, 0x00, 0x00, 0x00,
        0x01, 0x00,
        MAXIMUM_STILL_DISTANCE_GATE, 0x00, 0x00, 0x00,
        0x02, 0x00,
        UNMANNED_DURATION_S, 0x00, 0x00, 0x00,
        0x04, 0x03, 0x02, 0x01
    };
    if (!sendCommand(setNoOneCmd, sizeof(setNoOneCmd))) return false;
    if (!waitForAck(0x0160)) return false;

    // 3) Set Sensitivity
    const uint8_t setSensitivityCmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA,
        0x14, 0x00,
        0x64, 0x00,
        0x00, 0x00,
        0xFF, 0xFF, 0x00, 0x00,
        0x01, 0x00,
        SENSITIVITY, 0x00, 0x00, 0x00,
        0x02, 0x00,
        SENSITIVITY, 0x00, 0x00, 0x00,
        0x04, 0x03, 0x02, 0x01
    };
    if (!sendCommand(setSensitivityCmd, sizeof(setSensitivityCmd))) return false;
    if (!waitForAck(0x0164)) return false;

    // 4) End Configuration Mode
    const uint8_t endConfigCmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00,
        0x04, 0x03, 0x02, 0x01
    };
    if (!sendCommand(endConfigCmd, sizeof(endConfigCmd))) return false;
    if (!waitForAck(0x01FE)) return false;

    Serial.println("LD2410 successfully configured.");
    return true;
}

bool LD2410::sendCommand(const uint8_t* cmd, size_t len) {
    LD2410_SERIAL.flush(); // Clear any leftover data
    LD2410_SERIAL.write(cmd, len);
    LD2410_SERIAL.flush(); 
    return true;
}

bool LD2410::waitForAck(uint16_t expectedCmdWord, uint8_t expectedStatus) {
    // expectedCmdWord: the ACK will have Command Echo with lowest byte incremented by 1
    // e.g. if sent command was FF 00 => echo is FF 01. We'll store it as 0x01FF for convenience.

    uint8_t buffer[64];
    uint32_t start = millis();
    while((millis() - start) < 500) {
        if(LD2410_SERIAL.available() > 0) {
            size_t len = LD2410_SERIAL.readBytes(buffer, sizeof(buffer));
            if(len >= 8) {
                // Check frame header:
                if(buffer[0] == 0xFD && buffer[1] == 0xFC && buffer[2] == 0xFB && buffer[3] == 0xFA) {
                    uint16_t dataLen = buffer[4] | (buffer[5] << 8);
                    if(len >= (6 + dataLen)) {
                        uint8_t cc = buffer[6];
                        uint8_t ce = buffer[7];
                        uint16_t cmdEcho = (ce << 8) | cc;
                        uint8_t status = buffer[8];

                        // Check command echo and status
                        if(cmdEcho == expectedCmdWord && status == expectedStatus) {
                            // Looks like success
                            return true;
                        }
                    }
                }
            }
        }
    }

    Serial.println("ACK wait timed out or invalid ACK.");
    return false;
}

