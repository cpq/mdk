#include <sdk.h>

int main(void) {
  wdt_disable();
  gpio_output(LED1);
  gpio_input(BTN1);

  for (;;) {
    gpio_write(LED1, !gpio_read(BTN1));  // Assume pressed button is LOW
    delay_ms(10);
  }

  return 0;
}
