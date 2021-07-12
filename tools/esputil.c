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
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "slip.h"  // SLIP state machine logic

#define CHIP_ID_ESP32 0x00f01d83
#define CHIP_ID_ESP32_S2 0x000007c6
#define CHIP_ID_ESP32_C3_ECO_1_2 0x6921506f
#define CHIP_ID_ESP32_C3_ECO3 0x1b31506f
#define CHIP_ID_ESP8266 0xfff0c101

struct ctx {
  struct slip slip;  // SLIP state machine
  const char *baud;  // Baud rate
  const char *port;  // Serial port
  const char *fpar;  // Flash params, e.g. "0x220"
  bool verbose;      // Hexdump serial comms
  int fd;            // Serial port file descriptor
};

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

static void usage(void) {
  printf("Usage:\n");
  printf("  esputil [-v] [-b BAUD] [-p PORT] monitor\n");
  printf("  esputil [-v] [-b BAUD] [-p PORT] info\n");
  printf(
      "  esputil [-v] [-b BAUD] [-p PORT] [-fp FLASHPARAMS] "
      "flash OFFSET BINFILE ...\n");
  printf("  esputil mkbin OUTPUT.BIN ENTRYADDR SECTION_ADDR SECTION.BIN ...\n");
  exit(EXIT_FAILURE);
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

static int open_serial(const char *name, int baud, bool verbose) {
  struct termios tio;
  int fd = open(name, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    fail("open(%s): %d (%s)\n", name, fd, strerror(errno));
  } else if (tcgetattr(fd, &tio) == 0) {
    cfsetospeed(&tio, (speed_t) baud);
    cfsetispeed(&tio, (speed_t) baud);
    tio.c_lflag = tio.c_oflag = tio.c_iflag = 0;
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
    tio.c_cflag |= CLOCAL | CREAD | CS8;
    tcsetattr(fd, TCSANOW, &tio);
  }
  if (verbose) printf("Opened %s @ %d fd=%d\n", name, baud, fd);
  return fd;
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

uint8_t checksum2(uint8_t v, const uint8_t *buf, size_t len) {
  while (len--) v ^= *buf++;
  return v;
}

uint8_t checksum(const uint8_t *buf, size_t len) {
  return checksum2(0xef, buf, len);
}

// Execute serial command.
// Return 0 on sucess, or error code on failure
static int cmd(struct ctx *ctx, uint8_t op, void *buf, uint16_t len,
               uint32_t cs, int timeout_ms) {
  uint8_t tmp[8 + 16384];     // 8 is size of the header
  memset(tmp, 0, 8);          // Clear header
  tmp[1] = op;                // Operation
  memcpy(&tmp[2], &len, 2);   // Length
  memcpy(&tmp[4], &cs, 4);    // Checksum
  memcpy(&tmp[8], buf, len);  // Data

  slip_send(tmp, 8 + len, uart_tx, &ctx->fd);  // Send command
  if (ctx->verbose) dump("W", tmp, 8 + len);   // Hexdump if required

  fd_set rset = iowait(ctx->fd, timeout_ms);  // Wait until device is ready
  if (!FD_ISSET(ctx->fd, &rset)) return 1;    // Interrupted, fail
  int n = read(ctx->fd, tmp, sizeof(tmp));    // Read from a device
  if (n <= 0) fail("Serial line closed\n");  // Doh. Unplugged maybe?
  // if (verbose) dump("R", tmp, n);
  for (int i = 0; i < n; i++) {
    size_t r = slip_recv(tmp[i], &ctx->slip);  // Pass to SLIP state machine
    if (r == 0 && ctx->slip.mode == 0) putchar(tmp[i]);  // In serial mode
    if (r == 0) continue;
    if (ctx->verbose) dump("R", ctx->slip.buf, r);
    if (ctx->slip.buf[0] != 1 || ctx->slip.buf[1] != op) continue;
    if (r >= 10 && ctx->slip.buf[r - 2] != 0)
      return ctx->slip.buf[r - 1];  // esp8266
    if (r >= 12 && ctx->slip.buf[r - 4] != 0)
      return ctx->slip.buf[r - 3];  // esp32
    return 0;
  }
  return 2;
}

static int read_reg(struct ctx *ctx, uint32_t addr) {
  return cmd(ctx, 10, &addr, sizeof(addr), 0, 200);
}

// Assume chip is rebooted and is in download mode.
// Send SYNC commands until success
static bool chip_connect(struct ctx *ctx) {
  reset_to_bootloader(ctx->fd);
  for (int i = 0; i < 50; i++) {
    uint8_t data[36] = {7, 7, 0x12, 0x20};
    memset(data + 4, 0x55, sizeof(data) - 4);
    if (cmd(ctx, 8, data, sizeof(data), 0, 250) == 0) {
      usleep(50 * 1000);
      flushio(ctx->fd);  // Discard all data
      return true;
    }
    flushio(ctx->fd);
  }
  return false;
}

static void monitor(struct ctx *ctx) {
  fd_set rset = iowait(ctx->fd, 1000);
  if (FD_ISSET(ctx->fd, &rset)) {
    uint8_t buf[BUFSIZ];
    int n = read(ctx->fd, buf, sizeof(buf));   // Read from a device
    if (n <= 0) fail("Serial line closed\n");  // If serial is closed, exit

    if (ctx->verbose) dump("R", buf, n);
    for (int i = 0; i < n; i++) {
      size_t len = slip_recv(buf[i], &ctx->slip);            // Pass to SLIP
      if (len == 0 && ctx->slip.mode == 0) putchar(buf[i]);  // In serial mode
      if (len <= 0) continue;
      if (ctx->verbose) dump("SR", ctx->slip.buf, len);
    }
  }
  if (FD_ISSET(0, &rset)) {  // Forward stdin to a device
    uint8_t buf[BUFSIZ];
    int n = read(0, buf, sizeof(buf));
    for (int i = 0; i < n; i++) uart_tx(buf[i], &ctx->fd);
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

static void info(struct ctx *ctx) {
  if (!chip_connect(ctx)) fail("Error connecting\n");
  if (read_reg(ctx, 0x40001000)) fail("Error reading ID\n");
  uint32_t id = *(uint32_t *) &ctx->slip.buf[4];
  printf("Chip ID: 0x%x (%s)\n", id, chip_id_to_str(id));

  if (id == CHIP_ID_ESP32_C3_ECO3) {
    uint32_t efuse_base = 0x60008800;
    read_reg(ctx, efuse_base + 0x44);
    uint32_t mac0 = *(uint32_t *) &ctx->slip.buf[4];
    read_reg(ctx, efuse_base + 0x48);
    uint32_t mac1 = *(uint32_t *) &ctx->slip.buf[4];
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", (mac1 >> 8) & 255,
           mac1 & 255, (mac0 >> 24) & 255, (mac0 >> 16) & 255,
           (mac0 >> 8) & 255, mac0 & 255);
  }
}

static void flash(struct ctx *ctx, const char **args) {
  if (!chip_connect(ctx)) fail("Error connecting\n");

  if (read_reg(ctx, 0x40001000)) fail("Error reading ID\n");
  uint32_t chip_id = *(uint32_t *) &ctx->slip.buf[4];

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
      if (cmd(ctx, 13, d3, sizeof(d3), 0, 250)) fail("SPI attach failed\n");
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
    if (cmd(ctx, 2, d1, d1size, 0, 500)) fail("flash_begin failed\n");

    // Read from file into a buffer, but skip initial 16 bytes
    while ((n = fread(buf + hs, 1, block_size, fp)) > 0) {
      int oft = ftell(fp);
      printf("Writing %d @ 0x%x (%d%%)\n", n, flash_offset + oft - n,
             oft * 100 / size);

      // Embed flash params into an image
      // TODO(cpq): don't hardcode, detect them
      if (seq == 0) {
        uint16_t flash_params = (uint16_t) strtoul(ctx->fpar, NULL, 0);
        buf[hs + 2] = (uint8_t)((flash_params >> 8) & 255);
        buf[hs + 3] = (uint8_t)(flash_params & 255);

        // Set chip type in the extended header at offset 4.
        // Common header is 8, plus extended header offset 4 = 12
        if (chip_id == CHIP_ID_ESP32_C3_ECO3) buf[hs + 12] = 5;
        if (chip_id == CHIP_ID_ESP32_C3_ECO_1_2) buf[hs + 12] = 5;
      }

      // Flash write
      *(uint32_t *) &buf[0] = n;      // Populate initial bytes - size
      *(uint32_t *) &buf[4] = seq++;  // And sequence numner
      if (cmd(ctx, 3, buf, hs + n, checksum(buf + hs, n), 1500))
        fail("flash_data failed\n");
    }

    fclose(fp);
    args += 2;
  }

  // Flash end
  uint32_t d3[] = {0};  // 0: reboot, 1: run user code
  if (cmd(ctx, 4, d3, sizeof(d3), 0, 250)) fail("flash_end failed\n");

  hard_reset(ctx->fd);
}

static unsigned long align_to(unsigned long n, unsigned to) {
  return ((n + to - 1) / to) * to;
}

static void mkbin(const char *bin_path, const char *ep, const char *args[]) {
  FILE *bin_fp = fopen(bin_path, "w+b");
  if (bin_fp == NULL) fail("Cannot open %s: %s\n", bin_path, strerror(errno));
  uint32_t entrypoint = strtoul(ep, NULL, 16);

  uint8_t num_segments = 0;
  while (args[num_segments * 2] && args[num_segments * 2 + 1]) num_segments++;

  // Write common header
  uint8_t common_hdr[] = {0xe9, num_segments, 0, 0};
  fwrite(common_hdr, 1, sizeof(common_hdr), bin_fp);

  // Entry point
  fwrite(&entrypoint, 1, sizeof(entrypoint), bin_fp);

  // Extended header
  uint8_t extended_hdr[] = {0xee, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  fwrite(extended_hdr, 1, sizeof(extended_hdr), bin_fp);

  uint8_t cs = 0xef, zero = 0;

  // Iterate over segments
  for (uint8_t i = 0; i < num_segments; i++) {
    uint32_t load_address = strtoul(args[i * 2], NULL, 16);
    FILE *fp = fopen(args[i * 2 + 1], "rb");
    if (fp == NULL) fail("Cannot open %s: %d\n", args[i * 2 + 1], errno);
    fseek(fp, 0, SEEK_END);
    uint32_t size = ftell(fp);
    rewind(fp);
    uint32_t aligned_size = align_to(size, 4);

    fwrite(&load_address, 1, sizeof(load_address), bin_fp);
    fwrite(&aligned_size, 1, sizeof(aligned_size), bin_fp);

    uint8_t buf[BUFSIZ];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
      cs = checksum2(cs, buf, n);
      fwrite(buf, 1, n, bin_fp);
    }
    fclose(fp);
    for (uint8_t j = 0; j < aligned_size - size; j++) fputc(zero, bin_fp);
  }

  // Pad to 16 bytes and write checksum
  long ofs = ftell(bin_fp), aligned_ofs = align_to(ofs + 1, 16);
  for (uint8_t i = 0; i < aligned_ofs - ofs - 1; i++) fputc(zero, bin_fp);
  fputc(cs, bin_fp);

  fclose(bin_fp);
}

int main(int argc, const char **argv) {
  const char **command = NULL;  // Command to perform
  uint8_t slipbuf[32 * 1024];   // Buffer for SLIP context
  struct ctx ctx = {
      .port = getenv("PORT"),  // Serial port
      .baud = getenv("BAUD"),  // Serial port baud rate
      .fpar = "0x220",         // Flash params
      .slip = {.buf = slipbuf, .size = sizeof(slipbuf)},  // SLIP context
  };
  if (ctx.port == NULL) ctx.port = "/dev/ttyUSB0";
  if (ctx.baud == NULL) ctx.baud = "115200";

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      ctx.baud = argv[++i];
    } else if (strcmp(argv[i], "-fp") == 0 && i + 1 < argc) {
      ctx.fpar = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      ctx.port = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      ctx.verbose = true;
    } else if (argv[i][0] == '-') {
      usage();
    } else {
      command = &argv[i];
      break;
    }
  }
  if (!command || !*command) usage();

  // Commands that do not require serial port
  if (strcmp(*command, "mkbin") == 0) {
    if (!command[1] || !command[2] || !command[3] || !command[4]) usage();
    mkbin(command[1], command[2], &command[3]);
    return 0;
  }

  ctx.fd = open_serial(ctx.port, atoi(ctx.baud), ctx.verbose);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  if (strcmp(*command, "info") == 0) {
    info(&ctx);
  } else if (strcmp(*command, "flash") == 0) {
    flash(&ctx, &command[1]);
  } else if (strcmp(*command, "monitor") == 0) {
    while (s_signo == 0) monitor(&ctx);
  } else {
    printf("Unknown command: %s\n", *command);
    usage();
  }
  close(ctx.fd);
  return 0;
}
