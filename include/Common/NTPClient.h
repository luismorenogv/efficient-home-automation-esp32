/**
 * @file NTPClient.h
 * @brief Declaration of NTPClient class for NTP time synchronization
 *
 * @author Luis Moreno
 * @date Nov 25, 2024
 */

#pragma once

class NTPClient {
public:
    NTPClient();
    void initialize();
    bool isTimeValid();
};
