# ESP32 SDK

This is an alternative, make-based, bare metal SDK for the ESP32 chip.
It aims to be completely independent from the ESP-IDF (see dependencies
section below), and based on a Technical Reference Manuals:
[ESP32 C3 TRM](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf),
[ESP32 TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
.

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

Project Makefile should look like this:

```make
SOURCES = main.c another_file.c

EXTRA_CFLAGS ?=
EXTRA_LINKFLAGS ?=

include $(SDK_PATH)/make/build.mk
```

- Build one example: `make -C examples/blinky clean build flash monitor`
- Build all examples: `make -C tools test examples clean`

# API reference

All API are implemented from scratch using datasheet.

- GPIO
  ```c
  void gpio_output(int pin);              // Set pin mode to OUTPUT
  void gpio_input(int pin);               // Set pin mode to INPUT
  void gpio_write(int pin, bool value);   // Set pin to low (false) or high
  void gpio_toggle(int pin);              // Toggle pin value
  bool gpio_read(int pin);                // Read pin value
  ```
- SPI (software bit-bang using GPIO API)
  ```c
  // SPI descriptor. Specifies pins for MISO, MOSI, CLK and chip select
  struct spi { int miso, mosi, clk, cs[3]; };

  bool spi_init(struct spi *spi);           // Init SPI
  void spi_begin(struct spi *spi, int cs);  // Start SPI transaction
  void spi_end(struct spi *spi, int cs);    // End SPI transaction
  unsigned char spi_txn(struct spi *spi, unsigned char);   // Do SPI transaction
  ```
- UART
  ```c
  void uart_init(int no, int tx, int rx, int baud);   // Initialise UART
  bool uart_read(int no, uint8_t *c);   // Read byte. Return true on success
  void uart_write(int no, uint8_t c);   // Write byte. Block if FIFO is full
  ```
- LEDC
- WDT
  ```c
  void wdt_disable(void);   // Disable watchdog
  ```
- Timer
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
- System
  ```c
  int sdk_ram_used(void);           // Return used RAM in bytes
  int sdk_ram_free(void);           // Return free RAM in bytes
  unsigned long time_us(void);      // Return uptime in microseconds
  void delay_us(unsigned long us);  // Block for "us" microseconds
  void delay_ms(unsigned long ms);  // Block for "ms" milliseconds
  void spin(unsigned long count);   // Execute "count" no-op instructions
  ```
- Log
  ```c
  void sdk_log(const char *fmt, ...);   // Log message to UART 0
                                        // Supported specifiers:
                                        // %d, %x, %s, %p
  void sdk_hexdump(const void *buf, size_t len);  // Hexdump buffer
  ```
- TCP/IP

# ESP-IDF dependencies

This SDK aims to be ESP-IDF independent, however at this moment the following
dependencies are present:

- esptool.py script for flashing firmware and preparing image files

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
