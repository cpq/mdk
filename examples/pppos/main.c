#include <sdk.h>
#include "cell.h"

#define APN "isp.vodafone.ie"

int tx = 2, rx = 3, baud = 115200, bufsize = 2048;

static void txfn(const void *buf, int len) {
  const uint8_t *p = buf;
  while (len-- > 0) uart_write(1, *p++);
}

int main(void) {
  wdt_disable();

  // Allocate serial input buffer and cellular modem context
  char *buf = malloc((size_t) bufsize);
  const char *cmds[] = {"ATZ", "ATE0", "AT+CFUN=1", "AT+CPIN?", "AT+GSN", NULL};
  struct cell cell = {.buf = buf, .size = bufsize, .tx = txfn, .cmds = cmds};
  (void) cell;

  uart_init(1, tx, rx, baud);
  for (;;) {
    uint8_t c = 0;
    // if (uart_read(0, &c)) uart_write(1, c);
    // if (uart_read(1, &c)) uart_write(0, c);
    if (uart_read(1, &c)) cell_input(&cell, c);
    cell_poll(&cell, time_us() / 1000);
  }

  return 0;
}
