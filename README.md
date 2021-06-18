# ESP32 SDK

This is an alternative, make-based, bare metal SDK for the ESP32 C3 chip.
It aims to be completely independent from the ESP-IDF (see dependencies
section below), and based solely on the
[ESP32 C3 TRM](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf).

# Environment setup

Required tools:
- Esptool. Download from [esptool.py](https://raw.githubusercontent.com/espressif/esptool/master/esptool.py)
- GCC crosscompiler for riscv 32-bit:
   - MacOS (takes time):
      ```sh
      $ brew tap riscv/riscv
      $ brew install riscv-gnu-toolchain --with-multilib
      ```
   - Linux:
      ```sh
      $ sudo apt-get install -y gcc-riscv64-linux-gnu
      ```

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

A blinky example takes ~2 seconds to build and flash:

```sh
$ make -C examples/blinky clean build flash
```

# API reference

All API are implemented from scratch using datasheet.

- GPIO
- SPI (software bit-bang using GPIO API)
- UART
- LEDC
- WDT
- Timer
- System
- Log
- TCP/IP

# ESP-IDF dependencies

This SDK aims to be ESP-IDF independent, however at this moment the following
dependencies are present:

- esptool.py script for flashing firmware and preparing image files
- `boot/bootloader_*.bin` - 2nd stage bootloader
- `boot/partitions.bin` - a partition table (see below)

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
To support that, all hardware API are mocked out. The typical API
implementation looks like:

```c
#if defined(ESP32C3)
...
#elif defined(ESP32)
...
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
...  <-- Here goes a mocked-out hardware API implementation
#endif
```
