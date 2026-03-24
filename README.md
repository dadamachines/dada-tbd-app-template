# TBD-16 USB Mass Storage Firmware (with OLED)

USB Mass Storage Class (MSC) firmware for the TBD-16's RP2350. Exposes the SD card as a USB drive with OLED display feedback.

## Features

- **USB Mass Storage** — SD card appears as a USB drive on the host computer
- **OLED display feedback** — Status messages guide the user through each stage:
  - Boot: `DADA TBD-16` / `USB Mass Storage`
  - USB connected: `Mounting SD Card...` / `Please wait`
  - Host reads SD: `SD Card Ready` / `Eject before removing`
  - Eject: `Ejected safely` / `Rebooting...`
- **Auto-reboot on eject** — Device reboots automatically when the drive is ejected from the host

## Binary Type

This is a **no_flash** binary (`PICO_NO_FLASH=1`). The UF2 targets SRAM (0x20000000+) and is loaded by the TBD-16 bootloader directly from SD card into RAM.

## Build

Requires [PlatformIO](https://platformio.org/).

```bash
pio run
```

Output: `.pio/build/tusb-msc-oled/firmware.uf2`

## Architecture

This uses the **PlatformIO + picosdk** framework (raw Pico SDK, not Arduino) with:

- **no-OS-FatFS-SD-SDIO-SPI-RPi-Pico** — SDIO PIO driver + FatFS for SD card block access ([carlk3/no-OS-FatFS](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico), ctag-fh-kiel fork)
- **TinyUSB** — USB device stack (bundled with Pico SDK) for MSC + CDC
- **lcdspi** — SSD1309 OLED display driver (SPI, 128x64)

### TBD-16 Pin Mapping

| Function | GPIO |
|----------|------|
| SDIO CLK | 2 |
| SDIO CMD | 3 |
| SDIO DAT0 | 4 |
| SD Reset | 17 |
| OLED MOSI | 15 |
| OLED SCLK | 14 |
| OLED DC | 12 |
| OLED CS | 13 |
| OLED RST | 16 |
| USB-A Select | 11 |

## Credits

- SD card driver: [carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico) (Apache 2.0)
- TinyUSB MSC example: Ha Thach / tinyusb.org (MIT)
- OLED/UI board integration: ctag-fh-kiel

## License

MIT — see [LICENSE](LICENSE)
