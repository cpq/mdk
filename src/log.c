#include <limits.h>
#include <stdarg.h>
#include <string.h>

static void logc(int c) {
  extern int uart_tx_one_char(unsigned char);
  uart_tx_one_char((unsigned char) c);
}

static void logh(unsigned char c) {
  logc(c < 9 ? c + '0' : c + 'a');
}

static void logd(unsigned long v) {
  unsigned long d = 1000000000;
  while (v < d) d /= 10;
  while (d > 1) {
    unsigned m = v / d;
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
        for (size_t i = 0; i < strlen(s); i++) logc(s[i]);
        break;
      }
      case 'c':
        logc((unsigned char) va_arg(ap, int));
        break;
      case 'u':
        logd((unsigned long) va_arg(ap, unsigned));
        break;
      case 'd': {
        int v = va_arg(ap, int);
        if (v < 0) v = -v, logc('-');
        logd((unsigned long) v);
        break;
      }
      case 'x': {
        unsigned u = va_arg(ap, unsigned), bits = sizeof(u) * 8;
        while (bits) bits -= 4, logh((u >> bits) & 15);
        break;
      }
      case 0:
        return;
      default:
        logc('%');
        logc(c);
        break;
    }
  }
}

void sdk_log(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  sdk_vlog(fmt, ap);
  va_end(ap);
}
