/**
 * @file mac_addrs.h 
 * @brief Inludes the MAC address for each board
 * 
 * @author Luis Moreno
 * @date Nov 16, 2024
 */
#pragma once

#include <Arduino.h>

constexpr uint8_t lolin_d32_mac[6] = {0x24, 0x6F, 0x28, 0x2E, 0xA9, 0xC0};
constexpr uint8_t esp32s3_mac[6] = {0x3c, 0x84, 0x27, 0xe1, 0xb2, 0xcc};