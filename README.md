# Flash Nuke for TBD-16 (RP2350, copy-to-RAM)

A PlatformIO build of the [pico-universal-flash-nuke](https://github.com/Gadgetoid/pico-universal-flash-nuke) utility, configured for the dadamachines TBD-16's RP2350 co-processor.

Erases the entire flash chip, which is useful as a factory-reset / recovery tool.

## Why copy-to-RAM?

The original flash-nuke is built with `PICO_NO_FLASH` — the UF2 targets SRAM (`0x20000000`). This works when dragged onto a BOOTSEL drive, but **fails when flashed via Picoboot WebUSB** (the App Manager page), because Picoboot's `flashEraseAndWrite` only accepts flash addresses (`0x10000000`+).

This build uses `PICO_COPY_TO_RAM` instead: the binary is stored in flash, but CRT0 copies everything to SRAM before `main()` runs — so it can safely erase the flash it was loaded from. The resulting UF2 targets `0x10000000` and works with both delivery methods.

## Building

Requires [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation/index.html).

```
pio run -e flash-nuke
```

The output UF2 is at `.pio/build/flash-nuke/firmware.uf2`.

## How it works

- **`platformio.ini`** — Targets `generic_rp2350` with the `picosdk` framework. Sets `PICO_COPY_TO_RAM=1` and `PICO_FLASH_SIZE_BYTES` for 256 Mbit (32 MB) flash.
- **`copy_to_ram.py`** — PlatformIO post-script that patches the linker flags from `memmap_default.ld` to `memmap_copy_to_ram.ld` (the SDK's built-in copy-to-RAM linker script). Needed because PlatformIO's picosdk builder hardcodes the default linker script.
- **`src/nuke.c`** — The flash-erase logic from [Gadgetoid/pico-universal-flash-nuke](https://github.com/Gadgetoid/pico-universal-flash-nuke) (originally from Raspberry Pi examples, BSD-3-Clause). Erases all flash sectors, then reboots into USB bootloader.

## Source

Based on [Gadgetoid/pico-universal-flash-nuke](https://github.com/Gadgetoid/pico-universal-flash-nuke) by Philip Howard. The `nuke.c` source is from that repository (Copyright (c) 2020 Raspberry Pi (Trading) Ltd., BSD-3-Clause).

## License

[LGPL-3.0](LICENSE)
