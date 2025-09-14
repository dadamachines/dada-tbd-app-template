#pragma once

#include <atomic>
#include <string>
#include <ArduinoJson.h>
#include <Midi.h>

// defines for the CTAG TBD UI board with STM
#include <Wire.h>
#define I2C_SLAVE_ADDR 0x42
#define I2C_SDA 38
#define I2C_SCL 39

// defines for the OLED display, PIO SPI
#include <SoftwareSPI.h>
#include <Adafruit_GFX.h>
#include "DaDa_SSD1309.h"
#define OLED_MOSI 15
#define OLED_SCLK 14
#define OLED_DC 12
#define OLED_CS 13
#define OLED_RST 16

// neopixels for UI
#include <Adafruit_NeoPixel.h>
#define LED_COUNT 21
#define LED_PIN 26

typedef struct{
    uint16_t pot_adc_values[8]; // raw adc values
    // absolute positions of the pots, 0..1023
    uint16_t pot_positions[4]; // absolute position 0..1023
    // pot states, BIT0: fwd, BIT1: bwd, BIT2: fast (not implemented)
    uint8_t pot_states[4]; // BIT0: fwd, BIT1: bwd, BIT2: fast
    // step sequencer buttons
    uint16_t d_btns; // BIT0-15: D1-D16, buttons e.g. for step sequencer
    uint16_t d_btns_long_press; // BIT0-15: D1-D16
    // function buttons
    uint8_t f_btns; // BIT0: F1, BIT1: F2, BIT2: F3, BIT3: F4, BIT4: F5
    uint8_t f_btns_long_press; // BIT0: F1, BIT1: F2, BIT2: F3, BIT3: F4, BIT4: F5
    // system control buttons
    // 0: rec, 1: play, 2: stop, 3: page up, 4: page down, 5: up cursor, 6: left cursor, 7: down cursor, 8: right cursor, 9: endless pot 1 (leftmost), 10: endless pot 2, 11: endless pot 3, 12: endless pot 4 (rightmost)
    uint16_t mcl_btns;
    // BIT0: MCL1, BIT1: MCL2, BIT2: MCL3, BIT3: MCL4, BIT4: MCL5, BIT5: MCL6, BIT6: MCL7, BIT7: MCL8, BIT8: MCL9, BIT9: MCL10, BIT10: MCL11, BIT11: MCL12
    uint16_t mcl_btns_long_press;
    // BIT0: MCL1, BIT1: MCL2, BIT2: MCL3, BIT3: MCL4, BIT4: MCL5, BIT5: MCL6, BIT6: MCL7, BIT7: MCL8, BIT8: MCL9, BIT9: MCL10, BIT10: MCL11, BIT11: MCL12
    uint32_t systicks; // timestamp
} ui_data_t;

class Ui{
    Midi &midi;
    ui_data_t ui_data;
    uint32_t current_ui_data = 0; // current ui data index
    SoftwareSPI softSPI{SoftwareSPI(OLED_SCLK, OLED_DC, OLED_MOSI)};
    DaDa_SSD1309 display{DaDa_SSD1309(128, 64, &softSPI, OLED_DC, OLED_RST, OLED_CS)};
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
    unsigned long previousMillis = 0;
    const uint8_t rgb_led_rp2350 = 0;
    const uint8_t rgb_led_btn_map[16] = {8, 7, 6, 5, 4, 3, 2, 1, 9, 10, 11, 12, 13, 14, 15, 16};
    const uint8_t rgb_led_fbtn_map[3] = {19, 17, 18};
    const uint8_t rgb_led_mcl = 20;
    uint32_t ledStatus = 0;
    bool p4Ready{false}; // P4 ready indicator
    bool resetRequested{false}; // reset request indicator
    bool sdInitialized{false}; // SD card initialized indicator

    void displayString(const std::string& s);
    void displayStringWait1s(const std::string& s);

public:
    Ui(Midi &midi) : midi{midi} {}
    void Init();
    void InitHardware();
    void InitDisplay();
    void InitLeds();
    void InitSDCard();
    void Poll();
    void Update();
    bool UpdateUIInputs();
    void UpdateUIInputsBlocking();

    // examples
    void LoadDrumRackAndMapNoteOnsExample();
    void RealTimeCVTrigAPIExample();
    void BootIntoOTA1(); // can be used to reboot the P4 into sd-card mode (if available on the P4=

    // tests
    void RunUITests();
    void RunSpiAPITests();

    ui_data_t CopyUiData(){
        return ui_data;
    }
};
