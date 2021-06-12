#include <sdk.h>

int main(void) {
  wdt_disable();
  for (;;) {
    uint8_t c;
    if (uart_rx(&c) == 0) uart_tx(c);
  }
  return 0;
}
