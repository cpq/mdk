// Copyright (c) 2021 Cesanta
// All rights reserved

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
      "",
      progname, baud, port);
}

static void chip_reset(int fd) {
  int v;
  v = TIOCM_DTR, ioctl(fd, TIOCMBIC, &v);  // DTR -> false: IO0 -> HIGH
  v = TIOCM_RTS, ioctl(fd, TIOCMBIS, &v);  // RTS -> true: EN -> LOW
  usleep(1 * 1000);
  v = TIOCM_DTR, ioctl(fd, TIOCMBIS, &v);  // DTR -> true: IO0 -> LOW
  v = TIOCM_RTS, ioctl(fd, TIOCMBIC, &v);  // RTS -> false: EN -> HIGH
  usleep(50 * 1000);
  v = TIOCM_DTR, ioctl(fd, TIOCMBIC, &v);  // DTR -> false: IO0 -> HIGH
  tcflush(fd, TCIOFLUSH);                  // Discard all data
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

static bool cmd(uint8_t op, uint8_t *buf, uint16_t len, struct slip *slip,
                int timeout_ms, int fd, bool verbose) {
  uint8_t tmp[8 + 16384];            // 8 is size of the header
  uint32_t cs = checksum(buf, len);  // Calculate checksum
  memset(tmp, 0, 8);                 // Clear header
  tmp[1] = op;                       // Operation
  memcpy(&tmp[2], &len, 2);          // Length
  memcpy(&tmp[4], &cs, 4);           // Checksum
  memcpy(&tmp[8], buf, len);         // Data

  slip_send(tmp, 8 + len, uart_tx, &fd);  // Send command
  if (verbose) dump("W", tmp, 8 + len);   // Hexdump if required

  fd_set rset = iowait(fd, timeout_ms);      // Wait until device is ready
  if (!FD_ISSET(fd, &rset)) return false;    // Interrupted, fail
  int n = read(fd, tmp, sizeof(tmp));        // Read from a device
  if (n <= 0) fail("Serial line closed\n");  // Doh. Unplugged maybe?
  // if (verbose) dump("R", tmp, n);
  for (int i = 0; i < n; i++) {
    size_t r = slip_recv(tmp[i], slip);  // Pass to SLIP state machine
    if (r == 0 && slip->mode == 0) putchar(tmp[i]);  // In serial mode
    if (r == 0) continue;
    if (verbose) dump("R", slip->buf, r);
    if (slip->buf[0] != 1 || slip->buf[1] != op) continue;
    return true;
  }
  return false;
}

static bool read_reg(struct slip *slip, int fd, bool verbose, uint32_t addr) {
  return cmd(10, (uint8_t *) &addr, sizeof(addr), slip, 200, fd, verbose);
}

static bool write_reg(struct slip *slip, int fd, bool verbose, uint32_t addr,
                      uint32_t val, uint32_t mask, uint32_t delay_us) {
  uint32_t data[] = {addr, val, mask, delay_us};
  return cmd(9, (uint8_t *) data, sizeof(data), slip, 200, fd, verbose);
}

// Assume chip is rebooted and is in download mode.
// Send SYNC commands until success
static bool chip_connect(struct slip *slip, int fd, bool verbose) {
  for (int i = 0; i < 50; i++) {
    uint8_t data[36] = {7, 7, 0x12, 0x20};
    memset(data + 4, 0x55, sizeof(data) - 4);
    if (cmd(8, data, sizeof(data), slip, 250, fd, verbose)) {
      usleep(50 * 1000);
      tcflush(fd, TCIOFLUSH);  // Discard all data
      return true;
    }
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
  if (id == 0x00f01d83) return "ESP32";
  if (id == 0x000007c6) return "ESP32-S2";
  if (id == 0x6921506f) return "ESP32-C3-ECO1+2";
  if (id == 0x1b31506f) return "ESP32-C3-ECO3";
  if (id == 0xfff0c101) return "ESP8266";
  return "Unknown";
}

int main(int argc, char **argv) {
  const char *baud = "115200";        // Baud rate
  const char *port = "/dev/ttyUSB0";  // Serial port
  const char *command = NULL;         // Command to perform
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
      command = argv[i];
    }
  }
  if (command == NULL) usage(argv[0], baud, port);

  int fd = open_serial(port, atoi(baud), verbose);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialise SLIP state machine
  uint8_t slipbuf[32 * 1024];
  struct slip slip = {.buf = slipbuf, .size = sizeof(slipbuf)};
  if (strcmp(command, "info") == 0) {
    chip_reset(fd);
    if (!chip_connect(&slip, fd, verbose)) fail("Error connecting\n");
    if (!read_reg(&slip, fd, verbose, 0x40001000)) fail("Error reading ID\n");
    uint32_t id = *(uint32_t *) &slip.buf[4];
    printf("Chip ID: 0x%x (%s)\n", id, chip_id_to_str(id));

    // bool r2 = write_reg(&slip, fd, verbose, 0x40000000, 0xcafebabe,
    // 0xffffffff, 0);
    (void) write_reg;
  } else if (strcmp(command, "monitor") == 0) {
    while (s_signo == 0) monitor(&slip, fd, verbose);
    // printf("Exiting on signal %d\n", s_signo);
  } else {
    printf("Unknown command: %s\n", command);
    usage(argv[0], baud, port);
  }
  close(fd);
  return 0;
}
