/*
 * TBD-16 USB Mass Storage — RP2350
 *
 * Exposes the RP2350's SD card (SDIO) as a USB mass storage device.
 * The host computer sees it as a removable drive for file transfer.
 *
 * SD card wiring (SDIO, 4-bit):
 *   CLK  = GPIO 2
 *   CMD  = GPIO 3
 *   DAT0 = GPIO 4  (DAT1-3 = GPIO 5-7, auto-configured by SdFat)
 *
 * Build: PlatformIO, earlephilhower Arduino core, TinyUSB.
 * Targets RP2350 flash (0x10000000) — persistent across power cycles.
 *
 * MIT License — dadamachines GmbH
 */

#include <Arduino.h>
#include "SdFat.h"
#include "Adafruit_TinyUSB.h"

/* ── SDIO pin definitions (TBD-16 hardware) ── */
#define SDIO_CLK_GPIO  2
#define SDIO_CMD_GPIO  3
#define SDIO_DAT0_GPIO 4

/* ── SD card (SdFat with PIO SDIO) ── */
SdFat sd;

/* ── USB Mass Storage ── */
Adafruit_USBD_MSC usb_msc;

static bool sd_ready = false;

/* Forward declarations for MSC callbacks */
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize);
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize);
void msc_flush_cb(void);

void setup() {
    /* USB MSC descriptor */
    usb_msc.setID("dadamachines", "TBD-16 SD Card", "2.0");
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    usb_msc.setUnitReady(false);
    usb_msc.begin();

    /* Re-enumerate if already mounted (BOOTSEL -> flash -> run) */
    if (TinyUSBDevice.mounted()) {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }

    /* Initialize SD card via SDIO (4-bit, PIO) */
    sd_ready = sd.begin(SdioConfig(SDIO_CLK_GPIO, SDIO_CMD_GPIO, SDIO_DAT0_GPIO));

    if (sd_ready) {
        uint32_t block_count = sd.card()->sectorCount();
        usb_msc.setCapacity(block_count, 512);
        usb_msc.setUnitReady(true);
    }
}

void loop() {
    /* Nothing — USB MSC callbacks handle everything */
    delay(100);
}

/* ── MSC Callbacks ── */

int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize) {
    if (!sd_ready) return -1;
    uint32_t sectors = bufsize / 512;
    return sd.card()->readSectors(lba, (uint8_t *)buffer, sectors) ? (int32_t)bufsize : -1;
}

int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize) {
    if (!sd_ready) return -1;
    uint32_t sectors = bufsize / 512;
    return sd.card()->writeSectors(lba, buffer, sectors) ? (int32_t)bufsize : -1;
}

void msc_flush_cb(void) {
    sd.card()->syncDevice();
}
