#ifndef USE_TINYUSB_HOST
#error "Please Select USB Stack: Adafruit TinyUSB Host"
#else
#warning "All Serial1 Monitor Output is on Serial1"
#endif

#include "Midi.h"
static bool bShouldProcessMidi = false; // Flag to indicate if we should process MIDI messages, set by the WS sync interrupt

#include <Wire.h>
#define I2C_SLAVE_ADDR 0x42
#define I2C_SDA 38
#define I2C_SCL 39
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

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define OLED_MOSI 15
#define OLED_SCLK 14
#define OLED_DC 12
#define OLED_CS 13
#define OLED_RST 16
Adafruit_SH1106G *display;

#include "Adafruit_TinyUSB.h"
//#define LANGUAGE_ID 0x0409  // English

#define USBA_PWR_ENA_GPIO 10
#define USBA_SEL_GPIO 11

// Add USB MIDI Host support to Adafruit_TinyUSB
#include "usb_midi_host.h"

// USB Host object
Adafruit_USBH_Host USBHost;

// holding device descriptor
tusb_desc_device_t desc_device;

// holding the device address of the MIDI device
uint8_t midi_dev_addr = 0;

// sync
#define WS_PIN 27
unsigned long previousMillis = 0; 

#include <SPI.h>
#include <SoftwareSPI.h>
SoftwareSPI *softSPI;
#define SPI_SPEED 62500000 // 62.5 MHz
#define SPI_SCLK 34
#define SPI_MOSI 35
#define SPI_MISO 32
#define SPI_CS 33
#define SPI1_SCLK 30
#define SPI1_MOSI 31
#define SPI1_MISO 28
#define SPI1_CS 29
//#define SPI_BUFFER_LEN 64
#define SPI_BUFFER_LEN 512
SPISettings spiSettings(SPI_SPEED, MSBFIRST, SPI_MODE3);

// transfer structure is
// byte 0, 1 -> 0xCA 0xFE (fingerprint)
// byte 2 -> amount of data bytes (max. 64 - 3)
typedef struct{
    uint8_t out_buf[SPI_BUFFER_LEN], in_buf[SPI_BUFFER_LEN]; // actual buffers
} spi_trans_t;
spi_trans_t spi_trans[2];
uint32_t current_trans = 0;

#include <Adafruit_NeoPixel.h>
#define LED_COUNT 21
#define LED_PIN 26
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#define LED_GREEN 24
#include <atomic>
std::atomic_uint32_t ws_blink = 0;

const uint8_t rgb_led_rp2350 = 0;
const uint8_t rgb_led_btn_map[] = {8, 7, 6, 5, 4, 3, 2, 1, 9, 10, 11, 12, 13, 14, 15, 16};
const uint8_t rgb_led_fbtn_map[] = {19, 17, 18};
const uint8_t rgb_led_mcl = 20;


// the setup function runs once when you press reset or power the board
void setup()
{
  // SPI data init
  spi_trans[0].out_buf[0] = 0xCA; // fingerprint
  spi_trans[0].out_buf[1] = 0xFE; // fingerprint
  spi_trans[1].out_buf[0] = 0xCA; // fingerprint
  spi_trans[1].out_buf[1] = 0xFE; // fingerprint
  current_trans = 0;

  SPI1.setMISO(SPI1_MISO);
  SPI1.setMOSI(SPI1_MOSI);
  SPI1.setCS(SPI1_CS);
  SPI1.setSCK(SPI1_SCLK);
  SPI1.begin(true); // hw CS assertion

  Midi::Init(); // Initialize MIDI handling

  // WS sync to codec
  pinMode(WS_PIN, INPUT); // Configure button pin with pull-up resistor
  pinMode(LED_GREEN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(WS_PIN), ws_sync_cb, FALLING);

  // USB Host init
  pinMode(USBA_PWR_ENA_GPIO, OUTPUT);
  pinMode(USBA_SEL_GPIO, OUTPUT);
  digitalWrite(USBA_PWR_ENA_GPIO, true);
  digitalWrite(USBA_SEL_GPIO, true);
  Serial1.begin(115200); // All console prints go to UART0
  while (!Serial1) {
    delay(100);   // wait for Serial1 port to initialize
  }
  // Optionally, configure the buffer sizes here
  // The commented out code shows the default values
  // tuh_midih_define_limits(64, 64, 16);
  USBHost.begin(0); // 0 means use native RP2040 host

  Serial1.println("TinyUSB MIDI Host Example");
}

void setup1(){
  // UI STM32 communication
  Wire1.setSDA(I2C_SDA);
  Wire1.setSCL(I2C_SCL);
  Wire1.setClock(400000); 
  //Wire1.onFinishedAsync(i2c_async_done);
  Wire1.begin();

  // display init
  // TODO: Adafruit_SH1106G.cpp in Adafruit library, change l. 139, 140 _page_start_offset = 0 to avoid display line offset!
  softSPI = new SoftwareSPI(OLED_SCLK, OLED_DC, OLED_MOSI);
  display = new Adafruit_SH1106G(128, 64, softSPI, OLED_DC, OLED_RST, OLED_CS);
  display->begin(0, true);
  display->setRotation(2);
  display->clearDisplay();
  display->display();

  // NeoPixel init
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(10); 
}

void loop()
{
  // update midi host
  USBHost.task();
  bool connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);

}

void loop1(){
  
  static unsigned long tick = 0;
  unsigned long delta = millis() - tick;
  tick = millis();
  static uint32_t bpm = 0;
  // get data from stm
  Wire1.readAsync(I2C_SLAVE_ADDR, &ui_data, sizeof(ui_data_t), true);
  while(!Wire1.finishedAsync()) delay(1);
  char buf[64];
  
  // ws indicator
  if(ws_blink){
    strip.setPixelColor(rgb_led_rp2350, strip.Color(255, 255, 255));
    ws_blink = 0;
  }else{
    strip.setPixelColor(rgb_led_rp2350, strip.Color(0, 0, 0));
  }

  // print pots
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->setCursor(0, 0);
  display->printf("%04d %04d %04d %04d\n", ui_data.pot_positions[0], ui_data.pot_positions[1], ui_data.pot_positions[2], ui_data.pot_positions[3]);
  
  for(int i=0, j=0;i<4;i++){
    if(ui_data.pot_states[i] & (1 << 0)) buf[j++] = '1'; else buf[j++] = '0';
    if(ui_data.pot_states[i] & (1 << 1)) buf[j++] = '1'; else buf[j++] = '0';
  }
  buf[8] = 0;
  display->printf("%s\n", buf);

  // print dbuttons
  for(int i=0;i<16;i++){
    if(ui_data.d_btns & (1 << i)){
      buf[i] = '1'; 
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(0, 255, 0));
    }else{
      buf[i] = '0';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(64, 64, 64));
    }
    if(ui_data.d_btns_long_press & (1 << i)){
      buf[i] = 'L';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(255, 0, 0));
    }
  }
  buf[16] = 0;
  display->printf("%s\n", buf);

  // print fbuttons
  if (ui_data.f_btns & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(255, 0, 0));

  if (ui_data.f_btns & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(255, 0, 0)); 

  if (ui_data.f_btns & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(255, 0, 0)); 


  for(int i=0;i<5;i++){
    if(ui_data.f_btns & (1 << i)){
      buf[i] = '1'; 
    }
    else{
      buf[i] = '0';
    }
    if(ui_data.f_btns_long_press & (1 << i)){
      buf[i] = 'L';    
    } 
  }
  buf[5] = 0;
  display->printf("%s\n", buf);

  // print mcl buttons  
  if (ui_data.mcl_btns & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_mcl, strip.Color(64, 64, 64));
  if (ui_data.mcl_btns_long_press & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 0, 0));

  for(int i=0;i<13;i++){
    if(ui_data.mcl_btns & (1 << i)){
      buf[i] = '1'; 
    }
    else{
      buf[i] = '0';
    }
    if(ui_data.mcl_btns_long_press & (1 << i)){
      buf[i] = 'L';    
    } 
  }
  buf[13] = 0;
  display->printf("%s\n", buf);


  display->printf("FPS %dHz, MSPF %dms\n", 1000 / delta, delta);
  
  // 120 bpm indicator approx.
  if(bpm > 71){
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 255, 255));
    bpm = 0;
  }
  bpm++;
  
  display->display();
  strip.show();  
  
}

void ws_sync_cb(){
    // sync to word clock of codec i2s @ 44100Hz
    // divider 32 is block size of TBD
    static uint32_t cnt = 0;
    cnt++;
    if (cnt % 32 == 0){
      // is 44100Hz / 32 = 1378,125Hz or 725,62us
      if(SPI1.finishedAsync()) SPI1.endTransaction();
      SPI1.beginTransaction(spiSettings);
      SPI1.transferAsync(spi_trans[current_trans].out_buf, spi_trans[current_trans].in_buf, SPI_BUFFER_LEN);
      // swap buffers
      current_trans ^= 0x1;
      // process MIDI messages and get ready for next transfer
      Midi::Update(spi_trans[current_trans].out_buf + 2); // skip fingerprint bytes
    }
    
    // toggle indicator LED
    static bool led_state = false;
    if (cnt % 44100 == 0){
        ws_blink = 1;
        //digitalWrite(LED_GREEN, led_state);
        //led_state = !led_state;
    }
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb (uint8_t daddr)
{
  Serial1.printf("Device attached, address = %d\r\n", daddr);

  // Get Device Descriptor
  tuh_descriptor_get_device(daddr, &desc_device, 18, NULL, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr)
{
  Serial1.printf("Device removed, address = %d\r\n", daddr);
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx)
{
  Serial1.printf("MIDI device address = %u, IN endpoint %u has %u cables, OUT endpoint %u has %u cables\r\n",
      dev_addr, in_ep & 0xf, num_cables_rx, out_ep & 0xf, num_cables_tx);

  if (midi_dev_addr == 0) {
    // then no MIDI device is currently connected
    midi_dev_addr = dev_addr;
  }
  else {
    Serial1.printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  if (dev_addr == midi_dev_addr) {
    midi_dev_addr = 0;
    Serial1.printf("MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  }
  else {
    Serial1.printf("Unused MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  }
}

void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets)
{
  static bool led_state = false;
  if (midi_dev_addr == dev_addr) {
    if (num_packets != 0) {
      uint8_t cable_num;
      uint8_t buffer[48];
      while (1) {
        uint32_t bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, sizeof(buffer));
        if (bytes_read == 0) return;
        // blink a bit to indicate that we received a MIDI message
        digitalWrite(LED_GREEN, led_state);
        led_state = !led_state;
        Midi::QueueData(buffer, bytes_read); // queue the data for processing
      }
    }
  }
}