#include <sdk.h>

static void logc(int c) {
  // extern int uart_tx_one_char(int c);
  // uart_tx_one_char(c);
  uart_write(0, (unsigned char) c);
}

static void logs(const char *buf, size_t len) {
  for (size_t i = 0; i < len; i++) logc(buf[i]);
}

static unsigned char nibble(unsigned char c) {
  return c < 10 ? c + '0' : c + 'W';
}

#define ISPRINT(x) ((x) >= ' ' && (x) <= '~')
void sdk_hexdump(const void *buf, size_t len) {
  const unsigned char *p = buf;
  unsigned char ascii[16] = "", alen = 0;
  for (size_t i = 0; i < len; i++) {
    if ((i % 16) == 0) {
      // Print buffered ascii chars
      if (i > 0) logc('\t'), logs((char *) ascii, 16), logc('\n'), alen = 0;
      // Print hex address, then \t
      logc(nibble((i >> 12) & 15)), logc(nibble((i >> 8) & 15)),
          logc(nibble((i >> 4) & 15)), logc('0'), logc('\t');
    }
    logc(nibble(p[i] >> 4)), logc(nibble(p[i] & 15));  // Two nibbles, e.g. c5
    logc(' ');                                         // Space after hex number
    ascii[alen++] = ISPRINT(p[i]) ? p[i] : '.';        // Add to the ascii buf
  }
  while (alen < 16) logs("   ", 3), ascii[alen++] = ' ';
  logc('\t'), logs((char *) ascii, 16), logc('\n');
}

static void logx(unsigned long v) {
  unsigned bits = sizeof(v) * 8, show = 0;
  while (bits) {
    bits -= 4;
    unsigned char c = (v >> bits) & 15;
    if (show == 0 && c == 0) continue;
    logc(nibble(c));
    show = 1;
  }
  if (show == 0) logc('0');
}

static void logd(unsigned long v) {
  unsigned long d = 1000000000;
  while (v < d) d /= 10;
  while (d > 1) {
    unsigned long m = v / d;
    v -= m * d;
    d /= 10;
    logc((unsigned char) (m + '0'));
  }
  logc((unsigned char) (v + '0'));
}

void sdk_vlog(const char *fmt, va_list ap) {
  int c;
  while ((c = *fmt++) != '\0') {
    if (c != '%') {
      logc(c);
      continue;
    }
    c = *fmt++;
    switch (c) {
      case 's': {
        const char *s = va_arg(ap, char *);
        logs(s, strlen(s));
        break;
      }
      case 'c':
        logc((unsigned char) va_arg(ap, int));
        break;
      case 'u':
        logd((unsigned long) va_arg(ap, unsigned));
        break;
      case 'p':
        logc('0');
        logc('x');
        logx((unsigned long) va_arg(ap, void *));
        break;
      case 'd': {
        int v = va_arg(ap, int);
        if (v < 0) v = -v, logc('-');
        logd((unsigned long) v);
        break;
      }
      case 'x':
        logx(va_arg(ap, unsigned long));
        break;
      case 0:
        return;
      default:
        logc('%');
        logc(c);
        break;
    }
  }
}

void sdk_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  sdk_vlog(fmt, ap);
  va_end(ap);
}
