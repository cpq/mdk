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

#define ALIGN(a, b) (((a) + (b) -1) / (b) * (b))

struct chip {
  uint32_t id;  // Chip ID, stored in the ROM address 0x40001000
#define CHIP_ID_ESP32 0x00f01d83
#define CHIP_ID_ESP32_S2 0x000007c6
#define CHIP_ID_ESP32_C3_ECO_1_2 0x6921506f
#define CHIP_ID_ESP32_C3_ECO3 0x1b31506f
#define CHIP_ID_ESP8266 0xfff0c101
#define CHIP_ID_ESP32_S3_BETA2 0xeb004136
#define CHIP_ID_ESP32_S3_BETA3 0x9
#define CHIP_ID_ESP32_C6_BETA 0x0da1806f
  const char *name;  // Chpi name, e.g. "ESP32-S2"
  uint32_t bla;      // Bootloader flash offset
};

struct ctx {
  struct slip slip;  // SLIP state machine
  const char *baud;  // Baud rate, e.g. "115200"
  const char *port;  // Serial port, e.g. "/dev/ttyUSB0"
  const char *fpar;  // Flash params, e.g. "0x220"
  const char *fspi;  // Flash SPI pins: CLK,Q,D,HD,CS. E.g. "6,17,8,11,16"
  bool verbose;      // Hexdump serial comms
  int fd;            // Serial port file descriptor
  struct chip chip;  // Chip descriptor
};

static struct chip s_known_chips[] = {
    {0, "Unknown", 0},
    {CHIP_ID_ESP8266, "ESP8266", 0},
    {CHIP_ID_ESP32, "ESP32", 4096},
    {CHIP_ID_ESP32_C3_ECO_1_2, "ESP32-C3-ECO2", 0},
    {CHIP_ID_ESP32_C3_ECO3, "ESP32-C3-ECO3", 0},
    {CHIP_ID_ESP32_S2, "ESP32-S2", 0},
    {CHIP_ID_ESP32_S3_BETA2, "ESP32-S3-BETA2", 0},
    {CHIP_ID_ESP32_S3_BETA2, "ESP32-S3-BETA3", 0},
    {CHIP_ID_ESP32_S3_BETA2, "ESP32-C6-BETA", 0},
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

static void uart_tx(unsigned char ch, void *arg) {
  int fd = *(int *) arg;
  if (write(fd, &ch, 1) != 1) fail("failed to write %d to fd %d", ch, fd);
}

static void usage(struct ctx *ctx) {
  printf("Defaults: BAUD=%s, PORT=%s\n", ctx->baud, ctx->port);
  printf("Usage:\n");
  printf("  esputil [-v] [-b BAUD] [-p PORT] monitor\n");
  printf("  esputil [-v] [-b BAUD] [-p PORT] info\n");
  printf("  esputil [-v] [-b BAUD] [-p PORT] [-fp FLASH_PARAMS] ");
  printf("[-fspi FLASH_SPI] flash OFFSET BINFILE ...\n");
  // printf("  esputil [-v] [-b BAUD] [-p PORT] cmd CMD,DATA\n");
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

static void sleep_ms(int milliseconds) {
  usleep(milliseconds * 1000);
}

static void hard_reset(int fd) {
  set_dtr(fd, false);  // IO0 -> HIGH
  set_rts(fd, true);   // EN -> LOW
  sleep_ms(100);       // Wait
  set_rts(fd, false);  // EN -> HIGH
}

static void reset_to_bootloader(int fd) {
  sleep_ms(100);       // Wait
  set_dtr(fd, false);  // IO0 -> HIGH
  set_rts(fd, true);   // EN -> LOW
  sleep_ms(100);       // Wait
  set_dtr(fd, true);   // IO0 -> LOW
  set_rts(fd, false);  // EN -> HIGH
  sleep_ms(50);        // Wait
  set_dtr(fd, false);  // IO0 -> HIGH
  flushio(fd);         // Discard all data
}

// clang-format off
static speed_t termios_baud(int baud) {
    switch (baud) {
    case 9600:    return B9600;
    case 19200:   return B19200;
    case 38400:   return B38400;
    case 57600:   return B57600;
    case 115200:  return B115200;
    case 230400:  return B230400;
    case 460800:  return B460800;
    case 500000:  return B500000;
    case 576000:  return B576000;
    case 921600:  return B921600;
    case 1000000: return B1000000;
    case 1152000: return B1152000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 2500000: return B2500000;
    case 3000000: return B3000000;
    case 3500000: return B3500000;
    case 4000000: return B4000000;
    default:      return B0;
    }
}

static const char *ecode_to_str(int ecode) {
  switch (ecode) {
    case 5: return "Received message is invalid";
    case 6: return "Failed to act on received message";
    case 7: return "Invalid CRC in message";
    case 8: return "Flash write error";
    case 9: return "Flash read error" ;
    case 10: return "Flash read length error";
    case 11: return "Deflate error";
    default: return "Unknown error";
  }
}

static const char *cmdstr(int code) {
  switch (code) {
    case 2: return "FLASH_BEGIN";
    case 3: return "FLASH_DATA";
    case 4: return "FLASH_END";
    case 5: return "MEM_BEGIN";
    case 6: return "MEM_END" ;
    case 7: return "MEM_DATA";
    case 8: return "SYNC";
    case 9: return "WRITE_REG";
    case 10: return "READ_REG";
    case 11: return "SPI_SET_PARAMS";
    case 13: return "SPI_ATTACH";
    case 14: return "READ_FLASH_SLOW";
    case 15: return "CHANGE_BAUD_RATE";
    default: return "CMD_UNKNOWN";
  }
}
// clang-format on

static int open_serial(const char *name, int baud, bool verbose) {
  struct termios tio;
  int fd = open(name, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    fail("open(%s): %d (%s)\n", name, fd, strerror(errno));
  } else if (tcgetattr(fd, &tio) == 0) {
    tio.c_iflag = 0;             // input mode
    tio.c_oflag = 0;             // output mode
    tio.c_lflag = 0;             // local flags
    tio.c_cflag = CLOCAL | CS8;  // control flags
    // Order is important: setting speed must go after setting flags,
    // becase (depending on implementation) speed flags could reside in flags
    cfsetospeed(&tio, termios_baud(baud));
    cfsetispeed(&tio, termios_baud(baud));
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

  slip_send(tmp, 8 + len, uart_tx, &ctx->fd);        // Send command
  if (ctx->verbose) dump(cmdstr(op), tmp, 8 + len);  // Hexdump if required

  for (;;) {
    fd_set rset = iowait(ctx->fd, timeout_ms);  // Wait until device is ready
    if (!FD_ISSET(ctx->fd, &rset)) return 1;    // Interrupted, fail
    int n = read(ctx->fd, tmp, sizeof(tmp));    // Read from a device
    if (n <= 0) fail("Serial line closed\n");   // Doh. Unplugged maybe?
    // if (ctx->verbose) dump("--RAW_RESPONSE:", tmp, n);
    for (int i = 0; i < n; i++) {
      size_t r = slip_recv(tmp[i], &ctx->slip);  // Pass to SLIP state machine
      if (r == 0 && ctx->slip.mode == 0) putchar(tmp[i]);  // In serial mode
      if (r == 0) continue;
      if (ctx->verbose) dump("--SLIP_RESPONSE:", ctx->slip.buf, r);
      if (r < 10 || ctx->slip.buf[0] != 1 || ctx->slip.buf[1] != op) continue;
      // ESP8266's error indicator is in the 2 last bytes, ESP32's - last 4
      int eofs =
          ctx->chip.id == 0 || ctx->chip.id == CHIP_ID_ESP8266 ? r - 2 : r - 4;
      int ecode = ctx->slip.buf[eofs] ? ctx->slip.buf[eofs + 1] : 0;
      if (ecode) printf("error %d: %s\n", ecode, ecode_to_str(ecode));
      return ecode;
    }
  }
  return 42;
}

static int read32(struct ctx *ctx, uint32_t addr, uint32_t *value) {
  int ok = cmd(ctx, 10, &addr, sizeof(addr), 0, 100);
  if (ok == 0 && value != NULL) *value = *(uint32_t *) &ctx->slip.buf[4];
  return ok;
}

// Read chip ID from ROM and setup ctx->chip pointer
static void chip_detect(struct ctx *ctx) {
  if (read32(ctx, 0x40001000, &ctx->chip.id)) fail("Error reading chip ID\n");
  size_t i, nchips = sizeof(s_known_chips) / sizeof(s_known_chips[0]);
  for (i = 0; i < nchips; i++) {
    if (s_known_chips[i].id == ctx->chip.id) ctx->chip = s_known_chips[i];
  }
};

// Assume chip is rebooted and is in download mode.
// Send SYNC commands until success, and detect chip ID
static bool chip_connect(struct ctx *ctx) {
  reset_to_bootloader(ctx->fd);
  for (int i = 0; i < 50; i++) {
    uint8_t data[36] = {7, 7, 0x12, 0x20};
    memset(data + 4, 0x55, sizeof(data) - 4);
    if (cmd(ctx, 8, data, sizeof(data), 0, 250) == 0) {
      sleep_ms(50);
      flushio(ctx->fd);  // Discard all data
      chip_detect(ctx);
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
    fflush(stdout);
  }
  if (FD_ISSET(0, &rset)) {  // Forward stdin to a device
    uint8_t buf[BUFSIZ];
    int n = read(0, buf, sizeof(buf));
    for (int i = 0; i < n; i++) uart_tx(buf[i], &ctx->fd);
  }
}

static void info(struct ctx *ctx) {
  if (!chip_connect(ctx)) fail("Error connecting\n");
  printf("Chip ID: 0x%x (%s)\n", ctx->chip.id, ctx->chip.name);

  if (ctx->chip.id == CHIP_ID_ESP32_C3_ECO3) {
    uint32_t efuse_base = 0x60008800, mac0, mac1;
    read32(ctx, efuse_base + 0x44, &mac0);
    read32(ctx, efuse_base + 0x48, &mac1);
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", (mac1 >> 8) & 255,
           mac1 & 255, (mac0 >> 24) & 255, (mac0 >> 16) & 255,
           (mac0 >> 8) & 255, mac0 & 255);
  }
}

static void flash(struct ctx *ctx, const char **args) {
  if (!chip_connect(ctx)) fail("Error connecting\n");

  uint16_t flash_params = 0;
  if (ctx->fpar != NULL) flash_params = strtoul(ctx->fpar, NULL, 0);

  // For non-ESP8266, SPI attach is mandatory
  if (ctx->chip.id != CHIP_ID_ESP8266) {
    uint32_t pins = 0;
    if (ctx->fspi != NULL) {
      // 6,17,8,11,16 -> 0xb408446, like esptool does
      unsigned a = 0, b = 0, c = 0, d = 0, e = 0;
      sscanf(ctx->fspi, "%u,%u,%u,%u,%u", &a, &b, &c, &e, &d);
      pins = a | (b << 6) | (c << 12) | (d << 18) | (e << 24);
      // printf("-----> %u,%u,%u,%u,%u -> %x\n", a, b, c, d, e, pins);
    }
    uint32_t d3[] = {pins, 0};
    if (cmd(ctx, 13, d3, sizeof(d3), 0, 250)) fail("SPI_ATTACH failed\n");
    // flash_id, flash size, block_size, sector_size, page_size, status_mask
    uint32_t d4[] = {0, 4 * 1024 * 1024, 65536, 4096, 256, 0xffff};
    if (cmd(ctx, 11, d4, sizeof(d4), 0, 250)) fail("SPI_SET_PARAMS failed\n");

    // Load first word from the bootloader - flash params are encoded there,
    // in the last 2 bytes, see README.md in the repo root
    if (ctx->fpar == NULL) {
      uint32_t d5[] = {ctx->chip.bla, 16};
      if (cmd(ctx, 14, d5, sizeof(d5), 0, 2000) != 0) {
        printf("Error: can't read bootloader @ addr %#x\n", ctx->chip.bla);
      } else if (ctx->slip.buf[8] != 0xe9) {
        printf("Wrong magic for bootloader @ addr %#x\n", ctx->chip.bla);
      } else {
        flash_params = (ctx->slip.buf[10] << 8) | ctx->slip.buf[11];
      }
    }
  }
  printf("Using flash params %#hx\n", flash_params);

  // Iterate over arguments: FLASH_OFFSET FILENAME ...
  while (args[0] && args[1]) {
    uint32_t flash_offset = strtoul(args[0], NULL, 0);
    FILE *fp = fopen(args[1], "rb");
    if (fp == NULL) fail("Cannot open %s: %s\n", args[1], strerror(errno));
    fseek(fp, 0, SEEK_END);
    int seq = 0, n, size = ftell(fp);
    rewind(fp);

    uint32_t block_size = 4096, hs = 16;
    uint8_t buf[hs + block_size];  // Initial 16 bytes are for serial cmd
    memset(buf, 0, hs);            // Clear them

    // Flash begin. S2, S3, C3 chips have an extra 5th parameter.
    uint32_t encrypted = 0;
    uint32_t num_blocks = (size + block_size - 1) / block_size;
    uint32_t d1[] = {size, num_blocks, block_size, flash_offset, encrypted};
    uint16_t d1size = sizeof(d1) - 4;

    if (ctx->chip.id == CHIP_ID_ESP32_S2 ||
        ctx->chip.id == CHIP_ID_ESP32_S3_BETA2 ||
        ctx->chip.id == CHIP_ID_ESP32_S3_BETA3 ||
        ctx->chip.id == CHIP_ID_ESP32_C6_BETA ||
        ctx->chip.id == CHIP_ID_ESP32_C3_ECO_1_2 ||
        ctx->chip.id == CHIP_ID_ESP32_C3_ECO3)
      d1size += 4;

    printf("Erasing %d bytes @ %#x", size, flash_offset);
    fflush(stdout);
    if (cmd(ctx, 2, d1, d1size, 0, 15000)) fail("\nflash_begin/erase failed\n");

    // Read from file into a buffer, but skip initial 16 bytes
    while ((n = fread(buf + hs, 1, block_size, fp)) > 0) {
      int oft = ftell(fp);
      for (int i = 0; i < 100; i++) putchar('\b');
      printf("Writing %s, %d/%d bytes @ 0x%x (%d%%)", args[1], n, size,
             flash_offset + oft - n, oft * 100 / size);
      fflush(stdout);

      // Embed flash params into an image
      // TODO(cpq): don't hardcode, detect them
      if (seq == 0) {
        if (flash_offset == ctx->chip.bla && flash_params != 0) {
          buf[hs + 2] = (uint8_t) ((flash_params >> 8) & 255);
          buf[hs + 3] = (uint8_t) (flash_params & 255);
        }
        // Set chip type in the extended header at offset 4.
        // Common header is 8, plus extended header offset 4 = 12
        if (ctx->chip.id == CHIP_ID_ESP32_C3_ECO3) buf[hs + 12] = 5;
        if (ctx->chip.id == CHIP_ID_ESP32_C3_ECO_1_2) buf[hs + 12] = 5;
      }

      // Align buffer to block_size and pad with 0xff
      // memset(buf + hs + n, 255, sizeof(buf) - hs - n);
      // n = ALIGN(n, block_size);

      // Flash write
      *(uint32_t *) &buf[0] = n;      // Populate initial bytes - size
      *(uint32_t *) &buf[4] = seq++;  // And sequence numner
      uint8_t cs = checksum(buf + hs, n);
      if (cmd(ctx, 3, buf, hs + n, cs, 1500)) fail("flash_data failed\n");
    }

    for (int i = 0; i < 100; i++) printf("\b \b");
    printf("Written %s, %d bytes @ %#x\n", args[1], size, flash_offset);
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
    if (fp == NULL)
      fail("Cannot open %s: %s\n", args[i * 2 + 1], strerror(errno));
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
      .port = getenv("PORT"),          // Serial port
      .baud = getenv("BAUD"),          // Serial port baud rate
      .fpar = getenv("FLASH_PARAMS"),  // Flash parameters
      .fspi = getenv("FLASH_SPI"),     // Flash SPI pins
      .slip = {.buf = slipbuf, .size = sizeof(slipbuf)},  // SLIP context
      .chip = s_known_chips[0],                           // Set chip to unknown
  };
  if (ctx.port == NULL) ctx.port = "/dev/ttyUSB0";
  if (ctx.baud == NULL) ctx.baud = "115200";

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      ctx.baud = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      ctx.port = argv[++i];
    } else if (strcmp(argv[i], "-fp") == 0 && i + 1 < argc) {
      ctx.fpar = argv[++i];
    } else if (strcmp(argv[i], "-fspi") == 0 && i + 1 < argc) {
      ctx.fspi = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      ctx.verbose = true;
    } else if (argv[i][0] == '-') {
      usage(&ctx);
    } else {
      command = &argv[i];
      break;
    }
  }
  if (!command || !*command) usage(&ctx);

  // Commands that do not require serial port
  if (strcmp(*command, "mkbin") == 0) {
    if (!command[1] || !command[2] || !command[3] || !command[4]) usage(&ctx);
    mkbin(command[1], command[2], &command[3]);
    return 0;
  }

  // Commands that require serial port. First, open serial.
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
    usage(&ctx);
  }
  close(ctx.fd);
  return 0;
}
