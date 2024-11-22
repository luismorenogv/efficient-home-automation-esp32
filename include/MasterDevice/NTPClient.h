/**
 * @file NTPClient.h
 * @brief Declaration of NTPClient class for NTP time synchronization
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#pragma once

class NTPClient {
public:
    NTPClient();
    void initialize();
    bool isTimeValid();
};
