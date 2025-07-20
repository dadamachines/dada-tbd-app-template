#pragma once
#include "DaDa_SPI.h"
#include <atomic>

class Midi final{
    std::atomic<uint32_t> ledStatus {0};
    std::atomic<uint32_t> p4Alive {0}; // P4 ready status
    DaDa_SPI real_time_spi {spi1, 29, 31, 28, 30, 30000000};
public:
    void Init();
    void Update();
    uint32_t GetLedStatus() const {
        return ledStatus.load();
    }
    uint32_t GetP4AliveStatus() const{
        return p4Alive.load();
    }
};
