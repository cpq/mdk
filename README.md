# MDK (Minimal Development Kit) - a baremetal ESP32/ESP32C3 SDK

An bare metal, make-based SDK for the ESP32, ESP32C3 chips.
It is written from scratch using datasheets (
[ESP32 C3 TRM](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf),
[ESP32 TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
).
It is completely independent from the ESP-IDF and does not use any
ESP-IDF tools or files. The only tool required is a GCC crosscompiler.

A screenshot below demonstrates a [examples/c3ws2812](examples/c3ws2812)
RGB LED firmware flashed on a ESP32-C3-DevKitM-1 board. It takes < 2 seconds
for a full firmware rebuild and flash:

![](examples/c3ws2812/rainbow.gif)

# Environment setup

- Using Docker on Linux or Mac. Slower builds, but works off-the-shelf:
  ```sh
  $ export MDK=/path/to/mdk                 # Points to MDK directory
  $ export PATH=$PATH:$MDK/tools            # Add $MDK/tools to $PATH
  $ export PORT=/dev/cu.usb*                # Serial port for flashing
  ```
- Native MacOS (installation takes time):
  ```sh
  $ brew tap riscv/riscv
  $ brew install riscv-gnu-toolchain --with-multilib
  $ export MDK=/path/to/mdk                 # Points to MDK directory
  $ export PATH=$PATH:$MDK/tools            # Add $MDK/tools to $PATH
  $ export TOOLCHAIN=riscv64-unknown-elf    # $TOOLCHAIN-gcc must run GCC
  $ export PORT=/dev/cu.usb*                # Serial port for flashing
  ```
- Native Linux: install GCC, e.g. from https://github.com/espressif/crosstool-NG, then
  ```sh
  $ export MDK=/path/to/mdk                 # Points to MDK directory
  $ export PATH=$PATH:$MDK/tools            # Add $MDK/tools to $PATH
  $ export TOOLCHAIN=riscv32-esp-elf        # $TOOLCHAIN-gcc must run GCC
  $ export PORT=/dev/ttyUSB0                # Serial port for flashing
  ```

Verify setup by building and flashing a blinky example firmware.
From repository root, execute:

```sh
$ make -C examples/blinky clean build flash monitor
```

# Firmware Makefile

Firmware Makefile should look like this:

```make
SOURCES = main.c another_file.c

EXTRA_CFLAGS ?=
EXTRA_LINKFLAGS ?=

include $(MDK)/make/build.mk
```

# Environment reference

- **Environment / Makefile variables:**
  - `ARCH` - Architecture. Possible values: ESP32C3, ESP32. Default: ESP32C3
  - `TOOLCHAIN` - GCC binary prefix. Default: riscv64-unknown-elf
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
  - `make unix` - Build Mac/Linux executable firmware, see "UNIX mode" section below
- **SDK Preprocessor definitions:**
  - `LED1` - User LED pin. Default: 2
  - `BTN1` - User button pin. Default: 9


# API reference

API support matrix:

| Name    | GPIO | SPI | I2C | UART | WiFi | Timer | System | RTOS |
| ----    | ---- | --- | --- | ---- | ---- | ----- | ------ | ---- |
| ESP32C3 | yes  | yes |  -  |  yes |  -   |  yes  |  yes   | -    |
| ESP32   | yes  | yes |  -  |  -   |  -   |  yes  |  yes   | -    |

- GPIO [src/gpio.h](src/gpio.h)
  ```c
  void gpio_output(int pin);              // Set pin mode to OUTPUT
  void gpio_input(int pin);               // Set pin mode to INPUT
  void gpio_write(int pin, bool value);   // Set pin to low (false) or high
  void gpio_toggle(int pin);              // Toggle pin value
  bool gpio_read(int pin);                // Read pin value
  ```
- SPI [src/spi.h](src/spi.h), [src/spi.c](src/spi.c)
  ```c
  // SPI descriptor. Specifies pins for MISO, MOSI, CLK and chip select
  struct spi { int miso, mosi, clk, cs[3]; };

  bool spi_init(struct spi *spi);           // Init SPI
  void spi_begin(struct spi *spi, int cs);  // Start SPI transaction
  void spi_end(struct spi *spi, int cs);    // End SPI transaction
  unsigned char spi_txn(struct spi *spi, unsigned char);   // Do SPI transaction
  ```
- UART [src/uart.h](src/uart.h), [src/uart.c](src/uart.c)
  ```c
  void uart_init(int no, int tx, int rx, int baud);   // Initialise UART
  bool uart_read(int no, uint8_t *c);   // Read byte. Return true on success
  void uart_write(int no, uint8_t c);   // Write byte. Block if FIFO is full
  ```
- LEDC
- WDT [src/wdt.h](src/wdt.h)
  ```c
  void wdt_disable(void);   // Disable watchdog
  ```
- Timer [src/timer.h](src/timer.h)
  ```c
  struct timer {
    uint64_t period;       // Timer period in micros
    uint64_t expire;       // Expiration timestamp in micros
    void (*fn)(void *);    // Function to call
    void *arg;             // Function argument
    struct timer *next;    // Linkage
  };

  #define TIMER_ADD(head_, p_, fn_, arg_)
  void timers_poll(struct timer *head, uint64_t now);
  ```
- System  [src/sys.h](src/sys.h)
  ```c
  int sdk_ram_used(void);           // Return used RAM in bytes
  int sdk_ram_free(void);           // Return free RAM in bytes
  unsigned long time_us(void);      // Return uptime in microseconds
  void delay_us(unsigned long us);  // Block for "us" microseconds
  void delay_ms(unsigned long ms);  // Block for "ms" milliseconds
  void spin(unsigned long count);   // Execute "count" no-op instructions
  ```
- Log [src/log.h](src/log.h), [src/log.c](src/log.c)
  ```c
  void sdk_log(const char *fmt, ...);   // Log message to UART 0
                                        // Supported specifiers:
                                        // %d, %x, %s, %p
  void sdk_hexdump(const void *buf, size_t len);  // Hexdump buffer
  ```
- TCP/IP


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

# esputil

`esputil` is a command line tool for managing Espressif devices. It is a
replacement of `esptool.py`. `esputil` is written in C, its source code
is in [tools/esputil.c](tools/esputil.c). Below is a quick reference:

```sh
$ esputil -h
Defaults: BAUD=115200, PORT=/dev/ttyUSB0
Usage:
  esputil [-v] [-b BAUD] [-p PORT] monitor
  esputil [-v] [-b BAUD] [-p PORT] info
  esputil [-v] [-b BAUD] [-p PORT] [-fp FLASH_PARAMS] [-fspi FLASH_SPI] flash OFFSET BINFILE ...
  esputil mkbin OUTPUT.BIN ENTRYADDR SECTION_ADDR SECTION.BIN ...
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
described at https://github.com/espressif/esptool/wiki/Serial-Protocol.

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
   F1  - flash mode. 0: QIO, 1: QOUT, 2: DIO, 3: DOUT
   F2  - flash size (high 4 bits) and flash frequency (low 4 bits):
            size: 0: 1MB, 0x10: 2MB, 0x20: 4MB, 0x30: 8MB, 0x40: 16MB
            freq: 0: 40m, 1: 26m, 2: 20m, 0xf: 80m
   ENTRY - 4-byte entry point address in little endian
   C     - Chip ID. 0: ESP32, 5: ESP32C3
   V     - Chip revision
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
