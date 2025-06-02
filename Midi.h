#pragma once

#include <atomic>
class Midi final{
    std::atomic<uint32_t> ledStatus;
public:
    void Init();
    void Update();
    uint32_t GetLedStatus() const {
        return ledStatus.load();
    }
};
