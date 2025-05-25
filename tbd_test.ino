#ifndef USE_TINYUSB_HOST
#error "Please Select USB Stack: Adafruit TinyUSB Host"
#else
#warning "All Serial1 Monitor Output is on Serial1"
#endif

#include "MidiParser.h"
#include "Ui.h"

Ui ui; // UI handling

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

#include <SPI.h>
#define SPI1_SPEED 62500000 // 62.5 MHz
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


#define LED_GREEN 24
#include <atomic>
std::atomic_uint32_t ws_blink = 0;

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

  MidiParser::Init(); // Initialize MIDI handling

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
  ui.Init(); // Initialize UI handling
}

void loop()
{
  // update midi host
  USBHost.task();
  bool connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);
}

void loop1(){
  ui.Update(); // Update UI handling
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
      MidiParser::Update(spi_trans[current_trans].out_buf + 2); // skip fingerprint bytes
    }
    
    // toggle indicator LED
    static bool led_state = false;
    if (cnt % 44100 == 0){
        ui.WSSync(); // update UI with sync
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
        MidiParser::QueueData(buffer, bytes_read); // queue the data for processing
      }
    }
  }
}