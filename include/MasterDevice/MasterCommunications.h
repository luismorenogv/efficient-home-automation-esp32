/**
 * @file MasterCommunications.h
 * @brief Declaration of MasterCommunications class for MasterDevice communication
 * 
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include "Common/CommunicationsBase.h"

// Extends the CommunicatinsBase class tailoring to Master's needs
class MasterCommunications : public CommunicationsBase {
public:
    MasterCommunications();

private:
    void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) override;
};
