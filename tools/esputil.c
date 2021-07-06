// Copyright (c) 2021 Cesanta
// All rights reserved
//
// ESP serial protocol doc:
// https://github.com/espressif/esptool/wiki/Serial-Protocol

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "slip.h"  // SLIP state machine logic

#define CHIP_ID_ESP32 0x00f01d83
#define CHIP_ID_ESP32_S2 0x000007c6
#define CHIP_ID_ESP32_C3_ECO_1_2 0x6921506f
#define CHIP_ID_ESP32_C3_ECO3 0x1b31506f
#define CHIP_ID_ESP8266 0xfff0c101

static int s_signo;

void signal_handler(int signo) {
  s_signo = signo;
}

static int fail(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

static int open_serial(const char *name, int baud, bool verbose) {
  int fd = open(name, O_RDWR | O_NOCTTY);
  struct termios tio;
  if (fd < 0) fail("open(%s): %d (%s)\n", name, fd, strerror(errno));
  if (fd >= 0 && tcgetattr(fd, &tio) == 0) {
    tio.c_ispeed = tio.c_ospeed = baud;
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = tio.c_oflag = tio.c_iflag = 0;
    tcsetattr(fd, TCSANOW, &tio);
  }
  if (verbose) printf("Opened %s @ %d fd=%d\n", name, baud, fd);
  return fd;
}

static char *hexdump(const void *buf, size_t len, char *dst, size_t dlen) {
  const unsigned char *p = (const unsigned char *) buf;
  size_t i, idx, n = 0, ofs = 0;
  char ascii[17] = "";
  if (dst == NULL) return dst;
  memset(dst, ' ', dlen);
  for (i = 0; i < len; i++) {
    idx = i % 16;
    if (idx == 0) {
      if (i > 0 && dlen > n)
        n += (size_t) snprintf(dst + n, dlen - n, "  %s\n", ascii);
      if (dlen > n)
        n += (size_t) snprintf(dst + n, dlen - n, "%04x ", (int) (i + ofs));
    }
    if (dlen < n) break;
    n += (size_t) snprintf(dst + n, dlen - n, " %02x", p[i]);
    ascii[idx] = (char) (p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i]);
    ascii[idx + 1] = '\0';
  }
  while (i++ % 16) {
    if (n < dlen) n += (size_t) snprintf(dst + n, dlen - n, "%s", "   ");
  }
  if (n < dlen) n += (size_t) snprintf(dst + n, dlen - n, "  %s\n", ascii);
  if (n > dlen - 1) n = dlen - 1;
  dst[n] = '\0';
  return dst;
}

static void dump(const char *label, const uint8_t *buf, size_t len) {
  char tmp[len * 5 + 100];  // Hexdump buffer
  printf("%s [%d bytes]\n%s\n", label, (int) len,
         hexdump(buf, len, tmp, sizeof(tmp)));
}

static void uart_tx(unsigned char byte, void *arg) {
  int len = write(*(int *) arg, &byte, 1);
  (void) len;  // Shut up GCC
}

static void usage(const char *progname, const char *baud, const char *port) {
  fail(
      "Usage: %s [OPTIONS] [COMMAND]\n"
      "OPTIONS:\n"
      "  -b BAUD\t - serial speed. Default: %s\n"
      "  -p PORT\t - ESP serial port. Default: %s\n"
      "  -v\t\t - be verbose, hexdump serial data\n"
      "COMMAND:\n"
      "  info\t\t - show ESP chip info\n"
      "  flash A F ...\t - flash binary file F at flash offset A\n"
      "",
      progname, baud, port);
}

static void set_rts(int fd, bool value) {
  int v = TIOCM_RTS;
  ioctl(fd, value ? TIOCMBIS : TIOCMBIC, &v);
}

static void set_dtr(int fd, bool value) {
  int v = TIOCM_DTR;
  ioctl(fd, value ? TIOCMBIS : TIOCMBIC, &v);
}

static void flushio(int fd) {
  tcflush(fd, TCIOFLUSH);
}

static void hard_reset(int fd) {
  set_dtr(fd, false);  // IO0 -> HIGH
  set_rts(fd, true);   // EN -> LOW
  usleep(100 * 1000);  // Wait
  set_rts(fd, false);  // EN -> HIGH
}

static void reset_to_bootloader(int fd) {
  usleep(100 * 1000);  // Wait
  set_dtr(fd, false);  // IO0 -> HIGH
  set_rts(fd, true);   // EN -> LOW
  usleep(100 * 1000);  // Wait
  set_dtr(fd, true);   // IO0 -> LOW
  set_rts(fd, false);  // EN -> HIGH
  usleep(50 * 1000);   // Wait
  set_dtr(fd, false);  // IO0 -> HIGH
  flushio(fd);         // Discard all data
}

static fd_set iowait(int fd, int ms) {
  struct timeval tv = {.tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000};
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(fd, &rset);  // Listen to the UART fd
  FD_SET(0, &rset);   // Listen to stdin too
  if (select(fd + 1, &rset, 0, 0, &tv) < 0) FD_ZERO(&rset);
  return rset;
}

uint8_t checksum(const uint8_t *buf, size_t len) {
  uint8_t v = 0xef;
  while (len--) v ^= *buf++;
  return v;
}

// Execute serial command.
// Return 0 on sucess, or error code on failure
static int cmd(uint8_t op, void *buf, uint16_t len, uint32_t cs,
               struct slip *slip, int timeout_ms, int fd, bool verbose) {
  uint8_t tmp[8 + 16384];            // 8 is size of the header
  memset(tmp, 0, 8);                 // Clear header
  tmp[1] = op;                       // Operation
  memcpy(&tmp[2], &len, 2);          // Length
  memcpy(&tmp[4], &cs, 4);           // Checksum
  memcpy(&tmp[8], buf, len);         // Data

  slip_send(tmp, 8 + len, uart_tx, &fd);  // Send command
  if (verbose) dump("W", tmp, 8 + len);   // Hexdump if required

  fd_set rset = iowait(fd, timeout_ms);      // Wait until device is ready
  if (!FD_ISSET(fd, &rset)) return 1;        // Interrupted, fail
  int n = read(fd, tmp, sizeof(tmp));        // Read from a device
  if (n <= 0) fail("Serial line closed\n");  // Doh. Unplugged maybe?
  // if (verbose) dump("R", tmp, n);
  for (int i = 0; i < n; i++) {
    size_t r = slip_recv(tmp[i], slip);  // Pass to SLIP state machine
    if (r == 0 && slip->mode == 0) putchar(tmp[i]);  // In serial mode
    if (r == 0) continue;
    if (verbose) dump("R", slip->buf, r);
    if (slip->buf[0] != 1 || slip->buf[1] != op) continue;
    if (r >= 10 && slip->buf[r - 2] != 0) return slip->buf[r - 1];  // esp8266
    if (r >= 12 && slip->buf[r - 4] != 0) return slip->buf[r - 3];  // esp32
    return 0;
  }
  return 2;
}

static int read_reg(struct slip *slip, int fd, bool verbose, uint32_t addr) {
  return cmd(10, &addr, sizeof(addr), 0, slip, 200, fd, verbose);
}

// Assume chip is rebooted and is in download mode.
// Send SYNC commands until success
static bool chip_connect(struct slip *slip, int fd, bool verbose) {
  reset_to_bootloader(fd);
  for (int i = 0; i < 50; i++) {
    uint8_t data[36] = {7, 7, 0x12, 0x20};
    memset(data + 4, 0x55, sizeof(data) - 4);
    if (cmd(8, data, sizeof(data), 0, slip, 250, fd, verbose) == 0) {
      usleep(50 * 1000);
      flushio(fd);  // Discard all data
      return true;
    }
    flushio(fd);
  }
  return false;
}

static void monitor(struct slip *slip, int fd, bool verbose) {
  fd_set rset = iowait(fd, 1000);
  if (FD_ISSET(fd, &rset)) {
    uint8_t buf[BUFSIZ];
    int n = read(fd, buf, sizeof(buf));        // Read from a device
    if (n <= 0) fail("Serial line closed\n");  // If serial is closed, exit

    if (verbose) dump("R", buf, n);
    for (int i = 0; i < n; i++) {
      size_t len = slip_recv(buf[i], slip);  // Pass to SLIP state machine
      if (len == 0 && slip->mode == 0) putchar(buf[i]);  // In serial mode
      if (len <= 0) continue;
      if (verbose) dump("SR", slip->buf, len);
    }
  }
  if (FD_ISSET(0, &rset)) {  // Forward stdin to a device
    uint8_t buf[BUFSIZ];
    int n = read(0, buf, sizeof(buf));
    for (int i = 0; i < n; i++) uart_tx(buf[i], &fd);
  }
}

static const char *chip_id_to_str(uint32_t id) {
  if (id == CHIP_ID_ESP32) return "ESP32";
  if (id == CHIP_ID_ESP32_S2) return "ESP32-S2";
  if (id == CHIP_ID_ESP32_C3_ECO_1_2) return "ESP32-C3-ECO1+2";
  if (id == CHIP_ID_ESP32_C3_ECO3) return "ESP32-C3-ECO3";
  if (id == CHIP_ID_ESP8266) return "ESP8266";
  return "Unknown";
}

static void info(struct slip *slip, int fd, bool verbose) {
  if (!chip_connect(slip, fd, verbose)) fail("Error connecting\n");
  if (read_reg(slip, fd, verbose, 0x40001000)) fail("Error reading ID\n");
  uint32_t id = *(uint32_t *) &slip->buf[4];
  printf("Chip ID: 0x%x (%s)\n", id, chip_id_to_str(id));

  if (id == CHIP_ID_ESP32_C3_ECO3) {
    uint32_t efuse_base = 0x60008800;
    read_reg(slip, fd, verbose, efuse_base + 0x44);
    uint32_t mac0 = *(uint32_t *) &slip->buf[4];
    read_reg(slip, fd, verbose, efuse_base + 0x48);
    uint32_t mac1 = *(uint32_t *) &slip->buf[4];
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", (mac1 >> 8) & 255,
           mac1 & 255, (mac0 >> 24) & 255, (mac0 >> 16) & 255,
           (mac0 >> 8) & 255, mac0 & 255);
  }
}

static int flash(struct slip *slip, int fd, bool verbose, const char **args) {
  int args_consumed = 0;
  if (!chip_connect(slip, fd, verbose)) fail("Error connecting\n");

  if (read_reg(slip, fd, verbose, 0x40001000)) fail("Error reading ID\n");
  uint32_t chip_id = *(uint32_t *) &slip->buf[4];

  // Iterate over arguments: FLASH_OFFSET FILENAME ...
  while (args[0] && args[1]) {
    uint32_t flash_offset = strtoul(args[0], NULL, 0);
    FILE *fp = fopen(args[1], "rb");
    if (fp == NULL) fail("Cannot open %s: %s\n", args[1], strerror(errno));
    fseek(fp, 0, SEEK_END);
    int seq = 0, n, size = ftell(fp);
    rewind(fp);

    // For non-ESP8266, SPI attach is mandatory
    if (chip_id != CHIP_ID_ESP8266) {
      uint32_t d3[] = {0, 0};
      if (cmd(13, d3, sizeof(d3), 0, slip, 250, fd, verbose))
        fail("SPI attach failed\n");
    }

    uint32_t block_size = 4096, hs = 16;
    uint8_t buf[hs + block_size];  // Initial 16 bytes are for serial cmd
    memset(buf, 0, hs);            // Clear them

    // Flash begin. S2, S3, C3 chips have an extra 5th parameter.
    uint32_t encrypted = 0;
    uint32_t num_blocks = (size + block_size - 1) / block_size;
    uint32_t d1[] = {size, num_blocks, block_size, flash_offset, encrypted};
    uint16_t d1size = sizeof(d1) - 4;
    if (chip_id == CHIP_ID_ESP32_S2 || chip_id == CHIP_ID_ESP32_C3_ECO3)
      d1size += 4;
    if (cmd(2, d1, d1size, 0, slip, 500, fd, verbose))
      fail("flash_begin failed\n");

    // Read from file into a buffer, but skip initial 16 bytes
    while ((n = fread(buf + hs, 1, block_size, fp)) > 0) {
      int oft = ftell(fp);
      printf("Writing %d @ 0x%x (%d%%)\n", n, flash_offset + oft - n,
             oft * 100 / size);

      // Embed flash params into an image
      // TODO(cpq): don't hardcode, detect them
      if (seq == 0) buf[hs + 2] = 0x2, buf[hs + 3] = 0x20;

      // Flash write
      *(uint32_t *) &buf[0] = n;      // Populate initial bytes - size
      *(uint32_t *) &buf[4] = seq++;  // And sequence numner
      if (cmd(3, buf, hs + n, checksum(buf + hs, n), slip, 1500, fd, verbose))
        fail("flash_data failed\n");
    }

    fclose(fp);
    args_consumed += 2;
    args += 2;
  }

  // Flash end
  uint32_t d3[] = {0};  // 0: reboot, 1: run user code
  if (cmd(4, d3, sizeof(d3), 0, slip, 250, fd, verbose))
    fail("flash_end failed\n");

  hard_reset(fd);
  return args_consumed;
}

int main(int argc, const char **argv) {
  const char *baud = "115200";        // Baud rate
  const char *port = "/dev/ttyUSB0";  // Serial port
  const char **command = NULL;        // Command to perform
  bool verbose = false;               // Hexdump serial comms

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      baud = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      port = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = true;
    } else if (argv[i][0] == '-') {
      usage(argv[0], baud, port);
    } else {
      command = &argv[i];
      break;
    }
  }

  int fd = open_serial(port, atoi(baud), verbose);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialise SLIP state machine
  uint8_t slipbuf[32 * 1024];
  struct slip slip = {.buf = slipbuf, .size = sizeof(slipbuf)};

  while (command != NULL && *command != NULL) {
    if (strcmp(*command, "info") == 0) {
      info(&slip, fd, verbose);
    } else if (strcmp(*command, "flash") == 0) {
      command += flash(&slip, fd, verbose, &command[1]);
    } else if (strcmp(*command, "monitor") == 0) {
      while (s_signo == 0) monitor(&slip, fd, verbose);
    } else {
      printf("Unknown command: %s\n", *command);
      usage(argv[0], baud, port);
    }
    command++;
  }
  close(fd);
  return 0;
}
