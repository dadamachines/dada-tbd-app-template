#pragma once

#include <atomic>
class Midi final{
    std::atomic<uint32_t> ledStatus {0};
    std::atomic<uint32_t> p4Alive {0}; // P4 ready status
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
