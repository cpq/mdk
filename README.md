# A baremetal, single header ESP32/ESP32C3 SDK

A bare metal, single header make-based SDK for the ESP32/ESP32C3 chips.
Written from scratch using datasheets ( [ESP32 C3
TRM](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf),
[ESP32
TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)).
It is completely independent from the ESP-IDF and does not use any ESP-IDF
tools or files. It implements its own flashing utility `esputil` (see below),
written in C, with no dependencies or python or anything else.

A screenshot below demonstrates a [examples/ws2812](examples/ws2812)
RGB LED firmware flashed on a ESP32-C3-DevKitM-1 board. It takes < 2 seconds
for a full firmware rebuild and flash:

![](examples/ws2812/rainbow.gif)

Currently, "esp32c3" and "esp32" architectures are supported.
MDK file structure is as follows:

- $(ARCH)/[link.ld](esp32c3/link.ld) - a linker script file. ARCH is esp32 or esp32c3
- $(ARCH)/[boot.c](esp32c3/boot.c) - a startup code 
- $(ARCH)/[mdk.h](esp32c3/mdk.h) - a single header that implements MDK API
- $(ARCH)/[build.mk](esp32c3/build.mk) - a helper Makefile for building projects


# Environment setup

1. Use Linux or MacOS. Install Docker
2. Execute the following shell commands (or add them to your `~/.profile`):
  ```sh
  $ export MDK=/path/to/mdk     # Points to MDK directory
  $ export ARCH=esp32c3         # Valid choices: esp32 esp32c3
  $ export PORT=/dev/ttyUSB0    # Serial port for flashing
  ```

Verify setup by building and flashing a blinky example firmware.
From repository root, execute:

```sh
$ make -C examples/blinky clean build flash monitor
```

# Firmware Makefile

Firmware Makefile should look like this (see [examples/blinky/Makefile](examples/blinky/Makefile)):

```make
SOURCES = main.c another_file.c

EXTRA_CFLAGS ?=
EXTRA_LINKFLAGS ?=

include $(MDK)/$(ARCH)/build.mk
```

# Environment reference

- **Environment / Makefile variables:**
  - `ARCH` - Architecture. Possible values: esp32c3, esp32
  - `TOOLCHAIN` - Crosscompiler prefix. riscv64-unknown-elf or xtensa-esp32-elf
  - `PORT` - Serial port for flashing. Default: /dev/ttyUSB0
  - `FLASH_PARAMS` - Flash parameters, see below. Default: empty
  - `FLASH_SPI` - Flash SPI settings, see below. Default: empty
  - `EXTRA_CFLAGS` - Extra compiler flags. Default: empty
  - `EXTRA_LINKFLAGS` - Extra linker flags. Default: empty
- **Makefile targets:**
  - `make clean` - Clean up build artifacts
  - `make build` - Build firmware in a project's `build/` directory
  - `make flash` - Flash firmware. Needs PORT variable set
  - `make monitor` - Run serial monitor. Needs PORT variable set
- **Board defaults:** - overridable by e.g. `EXTRA_CFLAGS="-DLED1=3"`
  - `LED1` - User LED pin. Default: 2
  - `BTN1` - User button pin. Default: 9


# API reference

Currently, a limited API is implemented. The plan is to implement WiFi/BLE
primitives in order to integrate [cesanta/mongoose](https://github.com/cesanta/mongoose)
networking library. Unfortunately radio registers are not documented
by Espressif - please [contact us](https://mongoose.ws/contact/) if
you have more information on that.

- GPIO
  - `void gpio_output(int pin);` - set pin mode to OUTPUT
  - `void gpio_input(int pin);` - set pin mode to INPUT
  - `void gpio_write(int pin, bool value);` - set pin to low (false) or high
  - `void gpio_toggle(int pin);` - toggle pin value
  - `bool gpio_read(int pin);` - read pin value
- SPI
  - `struct spi { int miso, mosi, clk, cs; };` - an SPI descriptor
  - `bool spi_init(struct spi *spi);` - initialise SPI
  - `void spi_begin(struct spi *spi, int cs);` - start SPI transaction
  - `void spi_end(struct spi *spi, int cs);` - end SPI transaction
  - `uin8_t spi_txn(struct spi *spi, uint8_t);` - do SPI transaction: write one byte, read response
- UART 
  - `void uart_init(int no, int tx, int rx, int baud);` - initialise UART
  - `bool uart_read(int no, uint8_t *c);` - read byte. Return true on success
  - `void uart_write(int no, uint8_t c);` - write byte. Block if FIFO is full
- Misc
  - `void wdt_disable(void);` - disable watchdog
  - `uint64_t uptime_us(void);` - return uptime in microseconds
  - `void delay_us(unsigned long us);` - block for "us" microseconds
  - `void delay_ms(unsigned long ms);` - block for "ms" milliseconds
  - `void spin(unsigned long count);` - execute "count" no-op instructions

# esputil

`esputil` is a command line tool for managing Espressif devices. It is a
replacement of `esptool.py`. `esputil` is written in C, its source code
is in [tools/esputil.c](tools/esputil.c). It works on Linux, UNIX and Windows.
A pre-compiled Windows executable can be downloaded from
the [tools](tools) folder. Below is a quick reference:

```sh
$ esputil -h
Defaults: BAUD=115200, PORT=/dev/ttyUSB0
Usage:
  esputil [-v] [-b BAUD] [-p PORT] monitor
  esputil [-v] [-b BAUD] [-p PORT] info
  esputil [-v] [-b BAUD] [-p PORT] readmem ADDR SIZE
  esputil [-v] [-b BAUD] [-p PORT] readflash ADDR SIZE
  esputil [-v] [-b BAUD] [-p PORT] [-fp FLASH_PARAMS] [-fspi FLASH_SPI] flash OFFSET BINFILE ...
  esputil [-v] mkbin FIRMWARE.ELF FIRMWARE.BIN
  esputil mkhex ADDRESS1 BINFILE1 ADDRESS2 BINFILE2 ...
  esputil [-tmp TMP_DIR] unhex HEXFILE
```

Example: flash MDK-built ESP32C3 firmware:

```sh
$ esputil flash 0 ./build/firmware.bin
```

Example: flash ESP-IDF built firmware on ESP32-PICO-Kit board:

```sh
$ esputil -fspi 6,17,8,11,16 flash 
  0x1000 build/bootloader/bootloader.bin \
  0x8000 build/partitions.bin \
  0xe000 build/ota_data_initial.bin \
  0x10000 build/firmware.bin
```

## Build esputil

**Linux, macOS:**

```sh
$ make -C tools esputil
```

**Windows:**  
Choose one of the below compilers, and use the provided command from the mdk root folder. Check to make sure the path points to where you installed the compiler.

[Clang](https://releases.llvm.org/download.html): (From PowerShell)
```powershell
& 'C:\Program Files\LLVM\bin\clang.exe' -v -o esputil.exe tools\esputil.c
```

[TCC](http://download.savannah.gnu.org/releases/tinycc/): (From PowerShell)
```powershell
& 'C:\Program Files\tcc\tcc.exe' -v -o esputil.exe tools\esputil.c
```

MSVC: (From Developer Command Prompt)
```sh
cl tools\esputil.c
```

# ESP32 flashing

Flashing ESP32 chips is done via UART. In order to do so, ESP32 should be
rebooted in the flashing mode, by pulling IO0 low during boot. Then, a ROM
bootloader uses SLIP framing for a simple serial protocol, which is
described at https://docs.espressif.com/projects/esptool/en/latest/advanced-topics/serial-protocol.html.

Using that SLIP protocol, it is possible to write images to flash at
any offset. That is what [tools/esputil.c](tools/esputil.c) implements.
The image should be of the following format:

- COMMON HEADER - 4 bytes, contains number of segments in the image and flash params
- ENTRY POINT ADDRESS - 4 bytes, the beginning of the image code
- EXTENDED HEADER - 16 bytes, contains chip ID and extra flash params
- One or more SEGMENTS, which are padded to 16 bytes

```
 | COMMON HEADER |  ENTRY  |           EXTENDED HEADER          | SEGM1 | ... | 
 | 0xe9 N F1 F2  | X X X X | 0xee 0 0 0 C 0 V 0 0 0 0 0 0 0 0 1 |       | ... | 

   0xe9 - Espressif image magic number. All images must start with 0xe9
   N    - a number of segments in the image
   F1   - flash mode. 0: QIO, 1: QOUT, 2: DIO, 3: DOUT
   F2   - flash size (high 4 bits) and flash frequency (low 4 bits):
            size: 0: 1MB, 0x10: 2MB, 0x20: 4MB, 0x30: 8MB, 0x40: 16MB
            freq: 0: 40m, 1: 26m, 2: 20m, 0xf: 80m
   ENTRY - 4-byte entry point address in little endian
   C     - Chip ID. 0: ESP32, 5: ESP32C3
   V     - Chip revision

   Segment format: | ADDR | SIZE | DATA |
      ADDR  - segment load address
      SIZE  - segment size, aligned to 4 bytes
      DATA  - segment data, padded with 0 to 16-byte boundary
```

## Flash parameters

Image header format includes two bytes, `F1` and `F2`, which desribe
SPI flash parameters that ROM bootloader uses to load the rest of the firmware.
Those two bytes encode three parameters:

- Flash mode (F1 byte - can be `0`, `1`, `2`, `3`)
- FLash size (hight 4 bits of F2 byte - can be `0`, `1`, `2`, `3`, `4`)
- Flash frequency (low 4 bits of F2 byte - can be `0`, `1`, `2`, `f`)

By default, `esputil` fetches flash params `F1` and `F2` from the
existing bootloader by reading first 4 bytes of the bootloader from flash.
It is possible to manually set flash params via the `-fp`
flag, which is an integer value that represent 3 hex nimbles.
For example `fp 0x22f` sets flash to DIO, 4MB, 80MHz:

```sh
$ esputil -fp 0x22f flash 0 build/firmware.bin
```

## FLash SPI pin settings

Some boards fail to talk to flash: when you attempt to `esputil flash` them,
they'll time out with the `flash_begin/erase failed`, for example trying to
flash a bootloader on a ESP32-PICO-D4-Kit:


```sh
$ esputil flash 4096 build/bootloader/bootloader.bin 
Error: can't read bootloader @ addr 0x1000
Erasing 24736 bytes @ 0x1000
flash_begin/erase failed
```

This is because ROM bootloader on such boards have wrong SPI pins settings.
Espressif's `esptool.py` alleviates that by uploading its own piece of
software into ESP32 RAM, which does the right thing. `esputil` uses ROM
bootloader, and in order to fix an issue, a `-fspi FLASH_PARAMS` parameter
can be set which manually sets flash SPI pins. The format of the 
`FLASH_PARAMS` is five comma-separated integers for CLK,Q,D,HD,CS pins.

A previously failed ESP32-PICO-D4-Kit example can be fixed by passing
a correct SPI pin settings:

```sh
$ esputil -fspi 6,17,8,11,16 flash 4096 build/bootloader/bootloader.bin 
Written build/bootloader/bootloader.bin, 24736 bytes @ 0x1000
```

# Toolchain Docker images

By default, docker is used for builds. For `ARCH=esp32`, the `espressif/idf`
image is used. For `ARCH=esp32c3`, the `mdashnet/riscv` image is used,
which is built using the following Dockerfile:

```Dockerfile
FROM alpine:edge
RUN apk add --update build-base gcc-riscv-none-elf newlib-riscv-none-elf && rm -rf /var/cache/apk/*
```
