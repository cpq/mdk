#include <limits.h>
#include <stdarg.h>
#include <string.h>

static void logc(int c) {
  extern int uart_tx_one_char(unsigned char);
  uart_tx_one_char((unsigned char) c);
}

static void logx(unsigned long v) {
  unsigned bits = sizeof(v) * 8, show = 0;
  while (bits) {
    bits -= 4;
    unsigned char c = (v >> bits) & 15;
    if (show == 0 && c == 0) continue;
    logc(c < 10 ? c + '0' : c + 'W');
    show = 1;
  }
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

void sdk_log(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  sdk_vlog(fmt, ap);
  va_end(ap);
}
