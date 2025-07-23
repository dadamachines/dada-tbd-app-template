## What is this?
- An example project for the rp2350 of the DaDa TBD platform
- Covering:
  - USB MIDI host
  - UART MIDI
  - UI
    - OLED display
    - NeoPixel LED strip
    - Rotary encoders and buttons (thru polling STM uC firmware by I2C)
  - SD card initialisation

### Default operation
- MIDI data from all inputs is processed by MIDI parser
  - UART MIDI in 1, 2
  - USB MIDI host
  - USB MIDI device coming from SPI tunnel from ESP32-P4
- Parsed MIDI data is sent thru SPI to TBDs sound processor (ESP32-P4), this happens every 44100 / 32 Hz, i.e. each audio block
- OLED display shows current state of UI elements, i.e. button pressed, encoder positions, SD card status, etc.
- NeoPixel LED strip shows current state of UI elements, i.e. button pressed, encoder positions, etc.
- Rotary encoders and buttons are polled from STM32F0 uC via I2C

### Optional behaviour
- Call RealTimeCVTrigAPIExample() to see how to use the SPI API to send real-time control data without legacy midi parser
- Call LoadDrumRackAndMapNoteOnsExample() in Ui.cpp to load DrumRack plugin and map note-ons to it (refer to Ui::Init())
- Call RunSpiAPITests() in Ui.cpp to get an idea of the SPI API to change the plugin state on the TBD sound processor (ESP32-P4) (refer to Ui::Update())

### Communication interfaces
- Note that there are 2 SPI interfaces, which are both used to communicate with the TBD sound processor (ESP32-P4):
  - SPI1 is used for the real-time communication at 44100/32Hz, i.e. real-time control data for sound processor, such as note-ons, CCs, etc.
    - ESP32-P4 receives real-time plugin control data and sends USB MIDI device data, as well as audio level indicator data back to the RP2350
  - SPI0 is used for the API calls to change the plugin state, i.e. loading a plugin, changing parameters, etc.
- OLED is controlled via Software SPI and use of PIO
- NeoPixel LED strip is controlled via PIO
- USB host is used to connect USB MIDI devices, such as Arturia BeatStep Pro
  - USB host requires to manually enable the power and USB multiplexer via GPIOs, see Midi::Init() for details
- UART MIDI is used to connect external MIDI devices, such as MIDI keyboards, controllers, etc.

### Architecture / Pinout
[Pinmap](pinmap.md)

## How to build this project
### Use platformio!
- Install platformio
  - https://docs.platformio.org/en/latest/integration/ide/pioide.html#
- See debugging and further steps at https://docs.google.com/presentation/d/1Ueqo-6NZIdfJaPn4f1pV_JdaAnG25v9Co6zBZ0iWqKk/edit?usp=sharing


### Or native arduino-cli (note that you will need custom Adafruit TinyUSB library for this to work):
- Get dependencies:
    - adafruit/Adafruit GFX Library@^1.12.1
    - adafruit/Adafruit NeoPixel@^1.12.4
    - https://github.com/ctag-fh-kiel/Adafruit_TinyUSB_Arduino.git#fix/usb_host_arturia_bspro
    - https://github.com/rppicomidi/usb_midi_host.git#1.1.4
    - https://github.com/ctag-fh-kiel/DaDa_OLED.git#1.0.0
- Build with
  - cd src
  - arduino-cli compile --fqbn rp2040:rp2040:generic_rp2350:variantchip=RP2530B,usbstack=tinyusb_host   --build-path ./build
- Flash with cp build/src.uf2 /Volumes/RP2350

### Note
- You may need to delete the .pio folder, or the build folder respectively, if you switch between platformio and arduino-cli