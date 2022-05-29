#include <string.h>
#include "mdk_log.h"

static void logc(int c) {
  extern void uart_write(int, unsigned char);
  uart_write(0, (unsigned char) c);
}

int putchar(int c) {
  logc(c);
  return c;
}

static void logs(const char *buf, size_t len) {
  for (size_t i = 0; i < len; i++) logc(buf[i]);
}

static unsigned char nibble(unsigned char c) {
  return (unsigned char) (c < 10 ? c + '0' : c + 'W');
}

#define ISPRINT(x) ((x) >= ' ' && (x) <= '~')
void mdk_hexdump(const void *buf, size_t len) {
  const unsigned char *p = buf;
  unsigned char ascii[16] = "", alen = 0;
  for (size_t i = 0; i < len; i++) {
    if ((i % 16) == 0) {
      // Print buffered ascii chars
      if (i > 0) logs("  ", 2), logs((char *) ascii, 16), logc('\n'), alen = 0;
      // Print hex address, then \t
      logc(nibble((i >> 12) & 15)), logc(nibble((i >> 8) & 15)),
          logc(nibble((i >> 4) & 15)), logc('0'), logs("   ", 3);
    }
    logc(nibble(p[i] >> 4)), logc(nibble(p[i] & 15));  // Two nibbles, e.g. c5
    logc(' ');                                         // Space after hex number
    ascii[alen++] = ISPRINT(p[i]) ? p[i] : '.';        // Add to the ascii buf
  }
  while (alen < 16) logs("   ", 3), ascii[alen++] = ' ';
  logc('\t'), logs((char *) ascii, 16), logc('\n');
}
