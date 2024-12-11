/**
 * @file NTPClient.h
 * @brief Declaration of NTPClient class for NTP time synchronization
 * 
 * Manages synchronization of system time using NTP servers.
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

// Class to handle NTP time synchronization
class NTPClient {
public:
    NTPClient();
    
    // Initializes NTP client and synchronizes time
    void initialize();
    
    // Checks if the system time is valid
    bool isTimeValid();
};
