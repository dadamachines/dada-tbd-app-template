# Use platformio!
- See some steps at https://docs.google.com/presentation/d/1Ueqo-6NZIdfJaPn4f1pV_JdaAnG25v9Co6zBZ0iWqKk/edit?usp=sharing
- Install platformio
  - https://docs.platformio.org/en/latest/integration/ide/pioide.html#

# Or native arduino-cli (note that you will need custom Adafruit TinyUSB library for this to work):
- Get dependencies:
    - adafruit/Adafruit GFX Library@^1.12.1
    - adafruit/Adafruit NeoPixel@^1.12.4
    - https://github.com/ctag-fh-kiel/Adafruit_TinyUSB_Arduino.git#fix/usb_host_arturia_bspro
    - https://github.com/rppicomidi/usb_midi_host.git#1.1.4
- Build with
  - cd src
  - arduino-cli compile --fqbn rp2040:rp2040:generic_rp2350:variantchip=RP2530B,usbstack=tinyusb_host   --build-path ./build
- Flash with cp build/src.uf2 /Volumes/RP2350

# Note
- You may need to delete the .pio folder if you switch between platformio and arduino-cli