# ESP32 SDK

This is an alternative, make-based, bare metal SDK for the ESP32 chip.
Main source of inspiration is ESP32 TRM:
[esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

# Environment setup

Required tools:
- Esptool. Download from [esptool.py](https://raw.githubusercontent.com/espressif/esptool/master/esptool.py)
- GCC crosscompiler for riscv 32-bit:
   - MacOS (takes time): `brew tap riscv/riscv ; brew install riscv-gnu-toolchain --with-multilib`
   - Linux: ?

If all of your installs are inthe default locations, you can simply use the
built-in `export.sh` located in `tools`.

```sh
$ . tools/export.sh c3 r0
```

for example, where c3 is the chip type, and r0 is the chip rev, chip rev is
optional.

Alternatively, you can manually setup the environment by exporting the
following environment variables:

```sh
$ export ARCH=c3                        # Choices: c3, esp32
$ export TOOLCHAIN=riscv64-unknown-elf  # $TOOLCHAIN-gcc must resolve to GCC
$ export ESPTOOL=/path/to/esptool.py    # Full path to esptool.py
$ export PORT=/dev/ttyUSB0              # Serial port for flashing
```

Verify setup by running GCC:

```sh
$ $TOOLCHAIN-gcc --help
Usage: riscv32-esp-elf-gcc [options] file...
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

# UNIX mode

Firmware examples could be built on Mac/Linux as normal UNIX binaries.
In the firmware directory, type

```sh
make unix
```

That builds a `build/firmware` executable.
To support that, all hardware API like GPIO have UNIX implementation.

- instead of the toolchain's crosscompiler, a host's `$(CC)` compiler is used
  and a usual UNIX binary is built as a firmware file.
- Firmware uses `/dev/ptyp3` as a UART device. Note: so to "talk" to the
  firmware via UART, `/dev/ttyp3` should be used.

This helps to mock/test SDK or firmware functionality without an actual
hardware device. Specifically, TCP/IP stack functionality could be developed
and tested on a UNIX machine, provided an appropriate terminal utility
is available.
