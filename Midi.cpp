#include "Midi.h"
#include "MidiParser.h"

MidiParser midiparser; // MIDI handling

#define USBA_PWR_ENA_GPIO 10
#define USBA_SEL_GPIO 11

#include <SPI.h>
#define SPI1_SPEED 30000000 // 62.5 MHz
#define SPI1_SCLK 30
#define SPI1_MOSI 31
#define SPI1_MISO 28
#define SPI1_CS 29
//#define SPI_BUFFER_LEN 64
#define SPI_BUFFER_LEN 512

// sync codec 44100Hz
#define WS_PIN 27

#define LED_GREEN 24

static bool led_state = false;

static SPISettings spiSettings(SPI1_SPEED, MSBFIRST, SPI_MODE3);

// transfer structure is
// byte 0, 1 -> 0xCA 0xFE (fingerprint)
typedef struct{
    uint8_t out_buf[SPI_BUFFER_LEN], in_buf[SPI_BUFFER_LEN]; // actual buffers
} spi_trans_t;

static spi_trans_t spi_trans[2];
static uint32_t current_trans = 0;

#include "Adafruit_TinyUSB.h"
// Add USB MIDI Host support to Adafruit_TinyUSB
#include "usb_midi_host.h"

// USB Host object
static Adafruit_USBH_Host USBHost;

// holding device descriptor
static tusb_desc_device_t desc_device;

// holding the device address of the MIDI device
static uint8_t midi_dev_addr = 0;


#include <atomic>
std::atomic<uint32_t> ws_sync_counter{0}; // counter for word clock sync

static void ws_sync_cb(){
    // sync to word clock of codec i2s @ 44100Hz
    // divider 32 is block size of TBD
    static uint32_t cnt = 0;
    cnt++;
    if (cnt % 32 == 0){
        ws_sync_counter++;
    }
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr){
    // Get Device Descriptor
    tuh_descriptor_get_device(daddr, &desc_device, 18, NULL, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr){
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx){
    if (midi_dev_addr == 0){
        // then no MIDI device is currently connected
        midi_dev_addr = dev_addr;
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance){
    if (dev_addr == midi_dev_addr){
        midi_dev_addr = 0;
    }
}

void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets){
    if (midi_dev_addr == dev_addr){
        if (num_packets != 0){
            uint8_t cable_num;
            uint8_t buffer[48];
            while (1){
                uint32_t bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, sizeof(buffer));
                if (bytes_read == 0) return;
                // blink a bit to indicate that we received a MIDI message
                digitalWrite(LED_GREEN, led_state);
                led_state = !led_state;
                midiparser.QueueData(buffer, bytes_read); // queue the data for processing
            }
        }
    }
}

void Midi::Init(){
    midiparser.Init(); // Initialize MIDI handling

    // Optionally, configure the buffer sizes here
    // The commented out code shows the default values
    // tuh_midih_define_limits(64, 64, 16);
    USBHost.begin(0); // 0 means use native RP2040 host

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

    // WS sync to codec
    pinMode(WS_PIN, INPUT); // Configure button pin with pull-up resistor
    pinMode(LED_GREEN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(WS_PIN), ws_sync_cb, FALLING);

    // USB Host init
    pinMode(USBA_PWR_ENA_GPIO, OUTPUT);
    pinMode(USBA_SEL_GPIO, OUTPUT);
    digitalWrite(USBA_PWR_ENA_GPIO, true); // enable USB power
    digitalWrite(USBA_SEL_GPIO, true); // select USB A port
}

void Midi::Update(){
    // update midi host
    USBHost.task();
    bool connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);
    if (ws_sync_counter > 0){
        // is 44100Hz / 32 = 1378,125Hz or 725,62us
        if (SPI1.finishedAsync()) SPI1.endTransaction();
        SPI1.beginTransaction(spiSettings);
        SPI1.transferAsync(spi_trans[current_trans].out_buf, spi_trans[current_trans].in_buf, SPI_BUFFER_LEN);
        // swap buffers
        current_trans ^= 0x1;
        if (spi_trans[current_trans].in_buf[0] == 0xCA && spi_trans[current_trans].in_buf[1] == 0xFE){
            // fingerprint matches, we have a valid transfer
            // update the LED status from the SPI transfer
            uint32_t *led = (uint32_t *) &spi_trans[current_trans].in_buf[2];
            ledStatus = *led; // update led status from SPI transfer
            // see if we have USB device midi data from p4?
            uint32_t *midi_len = (uint32_t*) &spi_trans[current_trans].in_buf[6];
            uint8_t *midi_data = (uint8_t*) &spi_trans[current_trans].in_buf[10];
            midiparser.QueueData(midi_data, *midi_len);
        }
        midiparser.Update(spi_trans[current_trans].out_buf + 2); // skip fingerprint bytes
        // if we have a word clock sync, then we can update the MIDI parser
        ws_sync_counter = 0; // reset the counter
    }
}
