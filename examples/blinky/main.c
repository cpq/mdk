#include <sdk.h>

#define LED_PIN 1

int _start(void) {
  wdt_disable();
  gpio_output(LED_PIN);
  // gpio_write(LED_PIN, 1);
  // for (;;) asm("nop");

  for (;;) {
    gpio_write(LED_PIN, 0);
    delay_ms(250);
    gpio_write(LED_PIN, 1);
    delay_ms(250);
  }

  return 0;
}
