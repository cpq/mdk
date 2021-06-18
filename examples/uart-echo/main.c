#include <sdk.h>

int main(void) {
  wdt_disable();
  for (;;) {
    uint8_t c;
    if (uart_read(0, &c)) uart_write(0, c);
  }
  return 0;
}
