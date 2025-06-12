#include "DaDa_SSD1309_MCL.h"

#define SSD1309_SETPAGEADDR 0xB0 ///< Set page address command

DaDa_SSD1309_MCL::DaDa_SSD1309_MCL(uint16_t w, uint16_t h, SPIClass *spi,
                                 int16_t dc_pin, int16_t rst_pin,
                                 int16_t cs_pin, uint32_t bitrate)
    : Adafruit_GrayOLED(1, w, h, spi, dc_pin, rst_pin, cs_pin, bitrate) {}

DaDa_SSD1309_MCL::~DaDa_SSD1309_MCL(void) {}

void DaDa_SSD1309_MCL::display(void) {
  // ESP8266 needs a periodic yield() call to avoid watchdog reset.
  // With the limited size of SH110X displays, and the fast bitrate
  // being used (1 MHz or more), I think one yield() immediately before
  // a screen write and one immediately after should cover it.  But if
  // not, if this becomes a problem, yields() might be added in the
  // 32-byte transfer condition below.
  yield();

  // uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
  uint8_t *ptr = buffer;
  uint8_t dc_byte = 0x40;
  uint8_t pages = ((HEIGHT + 7) / 8);

  uint8_t bytes_per_page = WIDTH;

  /*
  Serial.print("Window: (");
  Serial.print(window_x1);
  Serial.print(", ");
  Serial.print(window_y1);
  Serial.print(" -> (");
  Serial.print(window_x2);
  Serial.print(", ");
  Serial.print(window_y2);
  Serial.println(")");
  */

  uint8_t first_page = window_y1 / 8;
  //  uint8_t last_page = (window_y2 + 7) / 8;
  uint8_t page_start = min(bytes_per_page, (uint8_t)window_x1);
  uint8_t page_end = (uint8_t)max((int)0, (int)window_x2);
  /*
  Serial.print("Pages: ");
  Serial.print(first_page);
  Serial.print(" -> ");
  Serial.println(last_page);
  pages = min(pages, last_page);

  Serial.print("Page addr: ");
  Serial.print(page_start);
  Serial.print(" -> ");
  Serial.println(page_end);
  */

  for (uint8_t p = first_page; p < pages; p++) {
    uint8_t bytes_remaining = bytes_per_page;
    ptr = buffer + (uint16_t)p * (uint16_t)bytes_per_page;
    // fast forward to dirty rectangle beginning
    ptr += page_start;
    bytes_remaining -= page_start;
    // cut off end of dirty rectangle
    bytes_remaining -= (WIDTH - 1) - page_end;

    if (i2c_dev) { // I2C
      uint16_t maxbuff = i2c_dev->maxBufferSize() - 1;

      uint8_t cmd[] = {
          0x00, (uint8_t)(SSD1309_SETPAGEADDR + p),
          (uint8_t)(0x10 + ((page_start + _page_start_offset) >> 4)),
          (uint8_t)((page_start + _page_start_offset) & 0xF)};

      // Set high speed clk
      i2c_dev->setSpeed(i2c_preclk);

      i2c_dev->write(cmd, 4);

      while (bytes_remaining) {
        uint8_t to_write = min(bytes_remaining, (uint8_t)maxbuff);
        i2c_dev->write(ptr, to_write, true, &dc_byte, 1);
        ptr += to_write;
        bytes_remaining -= to_write;
        yield();
      }

      // Set low speed clk
      i2c_dev->setSpeed(i2c_postclk);

    } else { // SPI
      uint8_t cmd[] = {
          (uint8_t)(SSD1309_SETPAGEADDR + p),
          (uint8_t)(0x10 + ((page_start + _page_start_offset) >> 4)),
          (uint8_t)((page_start + _page_start_offset) & 0xF)};

      digitalWrite(dcPin, LOW);
      spi_dev->write(cmd, 3);
      digitalWrite(dcPin, HIGH);
      spi_dev->write(ptr, bytes_remaining);
    }
  }
  // reset dirty window
  window_x1 = 1024;
  window_y1 = 1024;
  window_x2 = -1;
  window_y2 = -1;
}

bool DaDa_SSD1309_MCL::begin(uint8_t addr, bool reset) {

  Adafruit_GrayOLED::_init(addr, reset);

  _page_start_offset = 0; // shift left if needed

  // Init sequence, make sure its under 32 bytes, or split into multiples!
  static const uint8_t init[] = {
        0xDF, 0x12, // command lock
        0xAE, // display off
        0xD5, 0xA0, // set display clock divide ratio/oscillator frequency
        0xA8, 0x3F, // set multiplex ratio
        0xD3, 0x00, // set display offset
        0x40, // set display start line
        0xA1, // set segment re-map
        0xC8, // set COM output scan direction
        0xDA, 0x12, // set COM pins hardware configuration
        0x81, 0xDF, // set contrast control / current control
        0xD9, 0x82, // set pre-charge period
        0xDB, 0x34, // set VCOMH deselect level
        0xA4, // display on
        0xA6, // normal display
  };

  if (!oled_commandList(init, sizeof(init))) {
    return false;
  }

  delay(100);                     // 100ms delay recommended
  oled_command(0xAF); // 0xaf -> Display On

  return true; // Success
}