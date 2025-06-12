#pragma once

#include <atomic>
#include <string>
// defines for the CTAG TBD UI board with STM
#include <Wire.h>
#define I2C_SLAVE_ADDR 0x42
#define I2C_SDA 38
#define I2C_SCL 39

// defines for the OLED display, PIO SPI
#include <SoftwareSPI.h>
#include <Adafruit_GFX.h>
#include "DaDa_SSD1309_MCL.h"
#define OLED_MOSI 15
#define OLED_SCLK 14
#define OLED_DC 12
#define OLED_CS 13
#define OLED_RST 16

// neopixels for UI
#include <Adafruit_NeoPixel.h>
#define LED_COUNT 21
#define LED_PIN 26

class Ui {
    typedef struct{
        uint16_t pot_adc_values[8]; // raw adc values
        uint16_t pot_positions[4]; // absolute position 0..1023
        uint8_t pot_states[4]; // BIT0: fwd, BIT1: bwd, BIT2: fast
        uint16_t d_btns; // BIT0-15: D1-D16
        uint16_t d_btns_long_press; // BIT0-15: D1-D16
        uint8_t f_btns; // BIT0: F1, BIT1: F2, BIT2: F3, BIT3: F4, BIT4: F5
        uint8_t f_btns_long_press; // BIT0: F1, BIT1: F2, BIT2: F3, BIT3: F4, BIT4: F5
        uint16_t mcl_btns; // BIT0: MCL1, BIT1: MCL2, BIT2: MCL3, BIT3: MCL4, BIT4: MCL5, BIT5: MCL6, BIT6: MCL7, BIT7: MCL8, BIT8: MCL9, BIT9: MCL10, BIT10: MCL11, BIT11: MCL12
        uint16_t mcl_btns_long_press; // BIT0: MCL1, BIT1: MCL2, BIT2: MCL3, BIT3: MCL4, BIT4: MCL5, BIT5: MCL6, BIT6: MCL7, BIT7: MCL8, BIT8: MCL9, BIT9: MCL10, BIT10: MCL11, BIT11: MCL12
        uint32_t systicks; // timestamp
    } ui_data_t;
    ui_data_t ui_data;
    uint32_t current_ui_data = 0; // current ui data index
    SoftwareSPI *softSPI;
    DaDa_SSD1309_MCL *display;
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
    unsigned long previousMillis = 0;
    const uint8_t rgb_led_rp2350 = 0;
    const uint8_t rgb_led_btn_map[16] = {8, 7, 6, 5, 4, 3, 2, 1, 9, 10, 11, 12, 13, 14, 15, 16};
    const uint8_t rgb_led_fbtn_map[3] = {19, 17, 18};
    const uint8_t rgb_led_mcl = 20;
    uint32_t ledStatus = 0;
    bool p4Ready {false}; // P4 ready indicator
    bool resetRequested {false}; // reset request indicator

    void displayString(const std::string &s);
    void displayStringWait1s(const std::string &s);
    void updateUIInputs();
public:
   void Init();
   void Update();
   void RunUITests();
   void RunSpiAPITests();
   void SetLedStatus(uint32_t led){
       ledStatus = led;
   }
    void SetP4Ready(bool ready){
        p4Ready = ready;
    }
};
