#pragma once
#include "DaDa_SPI.h"
// Sec. 4.2.10.4. mutex, pg. 383 pi-pico reference https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
#include "pico/mutex.h"
#include <atomic>

#define N_CVS_TBD 240
#define N_TRIGS_TBD 60

class Midi final{
    std::atomic<uint32_t> ledStatus {0};
    std::atomic<uint32_t> p4Alive {0}; // P4 ready status
    DaDa_SPI real_time_spi {spi1, 29, 31, 28, 30, 30000000};
    bool bypassLegacyMidiParser {false}; // bypass legacy MIDI parser, use only USB MIDI parser
    mutex_t real_time_mutex; // mutex for real-time state buffer
    std::atomic<uint32_t> real_time_state_buffer_consumed {0};
    uint8_t real_time_state_buffer[N_CVS_TBD * 4 + N_TRIGS_TBD]; // buffer for real-time state, will be copied if not using legacy MIDI parser
public:
    void Init();
    void Update();
    void SetBypassLegacyMidiParser(bool bypass);
    uint32_t GetLedStatus() const {
        return ledStatus.load();
    }
    uint32_t GetP4AliveStatus() const{
        return p4Alive.load();
    }
    void AcquireRealTimeMutexBlocking() {
        mutex_enter_blocking(&real_time_mutex);
        real_time_state_buffer_consumed.store(0);
    }
    void ReleaseRealTimeMutex(){
        mutex_exit(&real_time_mutex);
    }
    void SetRealTimeTrig(uint8_t index, uint8_t value);
    void SetRealTimeCV(uint8_t index, float value);
    uint32_t IsRealTimeBufferConsumed() const {
        return real_time_state_buffer_consumed.load();
    }
};
