# ESP32 SDK

This is an alternative, make-based, bare metal SDK for the ESP32 chip.
Main source of inspiration is ESP32 TRM:
[esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

# Environment setup

Required tools: esptool and GCC. Install ESP-IDF and export the following
environment variables:

```sh
$ export TOOLCHAIN=/path-to-gcc/bin/PREFIX      # see note
$ export ARCH=esp32                             # choices: esp32, c3
$ export ESPTOOL=/path/to/esptool.py
$ export PORT=/dev/ttyUSB0                      # serial port for flashing
```

The `TOOLCHAIN` variable should be set so `$(TOOLCHAIN)-gcc` resolves to a
GCC binary.  E.g. on my workstation:

```sh
$ export TOOLCHAIN=$HOME/.espressif/tools/xtensa-esp32-elf/esp-2020r3-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf
```

# Project build

```sh
$ make -C examples/blinky clean build flash
```

# TODO

- Do a proper peripheral setup - enable WiFi radio
- Pull in cnip and attach to the WiFi radio IO
- Implement malloc/free that uses several memory regions - use all RAM we could
  For example, an unused CPU1 cache in IRAM
- Implement simple task switching (RTOS), to prevent busy loops resetting us
- Don't bother enabling 2nd core, C3 does not have it anyway
- Design and implement GPIO, SPI, I2C API

# Notes

- Currently, an application gets loaded to the `0x10000` address - that's
  the address where ROM 1st stage boot loader jumps to. Partitions are not
  used. Implement partitioning later on for OTA support
- ESP32 has complex memory structure, with several SROM chunks and several
  SRAM chunks. Data/instructions RAM are different, hence there is DRAM (data)
  and IRAM (code) regions. They are non-contiguous. See `ld/esp32.ld`
- ROM contains a lot of functions pre-burned on a chip. The implementation
  is hidden, but signatures and function addresses are published,
  see https://github.com/espressif/esp-idf/tree/master/components/esp_rom
- We can pull some implementation, like libc's `memset`, `strlen` and so
  on, from ROM. ROM's implementation uses newlib
- We cannot use ROM's `malloc`, cause we don't know what memory regions
  it is going to use

Partitions table used:
```csv
nvs,      data, nvs,     0x9000,    20K,
app0,     app,  ota_0,   0x10000,   1280K,
app1,     app,  ota_1,   ,          1280K,
spiffs,   data, spiffs,  ,          1M,
```
