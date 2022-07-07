# A baremetal, single header ESP32/ESP32C3 SDK

A bare metal, single header make-based SDK for the ESP32/ESP32C3 chips.
Written from scratch using datasheets ( [ESP32 C3
TRM](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf),
[ESP32
TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)).
It is completely independent from the ESP-IDF and does not use any ESP-IDF
tools or files. MDK implements its own flashing utility `esputil`, which is
developed in a separate repo https://github.com/esputil. Esputil is
written in C, with no dependencies on python or anything else, working on
Mac, Linux, and Windows as a static, singe no-dependencies executable.

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
  - `make build` - Build firmware in a project directory
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


# Toolchain Docker images

By default, docker is used for builds. For `ARCH=esp32`, the `espressif/idf`
image is used. For `ARCH=esp32c3`, the `mdashnet/riscv` image is used,
which is built using the following Dockerfile:

```Dockerfile
FROM alpine:edge
RUN apk add --update build-base gcc-riscv-none-elf newlib-riscv-none-elf && rm -rf /var/cache/apk/*
```
