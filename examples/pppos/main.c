#include <sdk.h>
#include "cell.h"

#define APN "isp.vodafone.ie"
#define ATGDCONT "AT+CGDCONT=1,\"IP\",\"" APN "\""
#define ATDT "ATDT*99***1#"

#define TX_PIN 2
#define RX_PIN 3
#define BAUD 115200
#define RXBUFSIZE 1600

static void txfn(const void *buf, size_t len) {
  const uint8_t *p = buf;
  while (len--) uart_write(1, *p++);
}

int main(void) {
  wdt_disable();

  // Allocate serial input buffer and cellular modem context
  char *buf = malloc(RXBUFSIZE);
  const char *cmds[] = {"ATH", "ATZ", "ATE0", "ATI", ATGDCONT, ATDT, NULL};
  struct cell cell = {.buf = buf, .size = RXBUFSIZE, .tx = txfn, .cmds = cmds};

  // Setup TCP/IP netif
  struct net_if netif = {.out = txfn, .mac = {0xd8, 0xa0, 0x1d, 1, 2, 3}};

  // Read modem UART in an infinite loop, and feed the stack
  uart_init(1, TX_PIN, RX_PIN, BAUD);
  for (;;) {
    uint8_t c = 0;
    // if (uart_read(0, &c)) uart_write(1, c);
    // if (uart_read(1, &c)) uart_write(0, c);
    // continue;
    if (uart_read(1, &c)) cell_input(&cell, c);
    cell_poll(&cell, time_us() / 1000);
    if (cell.state == CELL_PPP && cell.len > 0) {
      size_t n = ppp_input(&netif, cell.buf, cell.len);
      if (n > 0) memmove(cell.buf, cell.buf + n, cell.len - n), cell.len -= n;
    }
  }

  return 0;
}
