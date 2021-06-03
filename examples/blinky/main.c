#include <sdk.h>

static int led_pin = 2;

int main(void) {
  wdt_disable();
  gpio_output(led_pin);

  for (;;) {
    sdk_log("Free RAM1: %d\n", sdk_ram_free());
    char *p = malloc(1000);
    sdk_log("p: %p\n", p);
    gpio_toggle(led_pin);   // Blink an LED
    spin(2999999);          // Delay a bit
  }

  return 0;
}
