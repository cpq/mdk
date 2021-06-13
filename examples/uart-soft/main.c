#include <sdk.h>

int main(void) {
  wdt_disable();

  int led = 1, tx = 3, rx = 2, baud = 115200;
  gpio_output(led);
  gpio_output(tx);
  gpio_input(rx);

  for (;;) {
    uint8_t c;
    if (uart_rx(&c) == 0) uart_tx_pin(tx, baud, c);
    if (uart_rx_pin(rx, baud, &c) == 0) uart_tx(c);
  }

  return 0;
}
