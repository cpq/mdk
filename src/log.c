#include <stdarg.h>
#include <string.h>

static void logc(int c) {
  extern int uart_tx_one_char(unsigned char);
  uart_tx_one_char((unsigned char) c);
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
      case 'x': {
        // TODO(cpq): If we enable this, chip resets... Figure out and fix
        // const char *hex = "0123456789abcdef";
        // unsigned u = va_arg(ap, unsigned), bits = sizeof(u) * 8;
        // while (bits) bits -= 4, logc(hex[(u >> bits) & 15]);
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
