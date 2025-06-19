#pragma once

// adapted from Adafruit_SH110X library
/*
*Software License Agreement (BSD License)

Copyright (c) 2012, Adafruit Industries
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holders nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Adafruit_GrayOLED.h>

#define SSD1309_BLACK 0   ///< Draw 'off' pixels
#define SSD1309_WHITE 1   ///< Draw 'on' pixels
#define SSD1309_INVERSE 2 ///< Invert pixels

class DaDa_SSD1309_MCL : public Adafruit_GrayOLED{
public:
    DaDa_SSD1309_MCL(uint16_t w, uint16_t h, SPIClass *spi, int16_t dc_pin,
                  int16_t rst_pin, int16_t cs_pin,
                  uint32_t bitrate = 10000000UL);
    virtual ~DaDa_SSD1309_MCL(void);

    void display(void);
    bool begin(uint8_t i2caddr = 0x3C, bool reset = true);

protected:
    /*! some displays are 'inset' in memory, so we have to skip some memory to
     * display */
    uint8_t _page_start_offset = 0;
};