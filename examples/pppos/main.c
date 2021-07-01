#include <sdk.h>
#include "cell.h"

#define GDCONT "AT+CGDCONT=1,\"IP\",\"iot.1nce.net\""
#define CELL_UART 1
#define CELL_TX 2
#define CELL_RX 3
#define CELL_BAUD 115200
#define RXBUFSIZE 1600

static void txfn(const void *buf, size_t len) {
  const uint8_t *p = buf;
  while (len--) uart_write(CELL_UART, *p++);
}

int main(void) {
  wdt_disable();

  // Allocate serial input buffer and cellular modem context
  struct cell cell = {.buf = malloc(RXBUFSIZE),
                      .size = RXBUFSIZE,
                      .tx = txfn,
                      .cmds = "ATZ;ATE0;ATI;AT+CIMI;" GDCONT ";ATDT*99***1#",
                      .dbg = sdk_log};

  // Setup TCP/IP netif
  struct net_if netif = {.out = txfn, .mac = {0xd8, 0xa0, 0x1d, 1, 2, 3}};

  // Read modem UART in an infinite loop, and feed the stack
  uart_init(CELL_UART, CELL_TX, CELL_RX, CELL_BAUD);
  for (;;) {
    uint8_t c = 0;
    while (uart_read(0, &c)) uart_write(CELL_UART, c);
#if 0
    while (uart_read(CELL_UART, &c)) uart_write(0, c);
    continue;
#endif
    while (uart_read(CELL_UART, &c)) cell_input(&cell, c);
    cell_poll(&cell, time_us() / 1000);
    if (cell.state == CELL_OK && cell.len > 0) {
      size_t n = ppp_input(&netif, cell.buf, cell.len);
      if (n > 0) memmove(cell.buf, cell.buf + n, cell.len - n), cell.len -= n;
    }
  }

  return 0;
}
