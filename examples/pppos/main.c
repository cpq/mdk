#include <sdk.h>
#include "cell.h"

int tx = 3, rx = 2, rst = 0, baud = 115200, bufsize = 2048;

static void txfn(const void *buf, int len) {
  const uint8_t *p = buf;
  while (len-- > 0) uart_tx_pin(tx, baud, *p++);
}

int main(void) {
  wdt_disable();

  gpio_output(LED1);
  gpio_input(BTN1);

  gpio_output(tx);
  gpio_output(rst);
  gpio_input(rx);

  // Allocate serial input buffer and cellular modem context
  char *buf = malloc((size_t) bufsize);
  const char *cmds[] = {
      "ATZ",          "ATE0",    "AT+CFUN=0",
      "ATH",          "ATI",     "AT+GSN",
      "AT+CIMI",      "AT+CCID", "AT+CREG=0",
      "AT+CREG?",     "AT+CSQ",  "AT+CGDCONT=1,\"IP\",\"isp.vodafone.ie\"",
      "ATDT*99***1#", NULL};
  struct cell cell = {.buf = buf, .size = bufsize, .tx = txfn, .cmds = cmds};

  for (;;) {
    uint8_t c;
    if (uart_rx(&c) == 0) uart_tx_pin(tx, baud, c);
    if (uart_rx_pin(rx, baud, &c) == 0) cell_rx(&cell, c);
    // if (uart_rx_pin(rx, baud, &c) == 0) uart_tx(c);

    cell_poll(&cell, time_us() / 1000);

    if (gpio_read(BTN1) == 0)
      gpio_write(rst, 0), delay_ms(10), gpio_write(rst, 1);
  }

  return 0;
}
