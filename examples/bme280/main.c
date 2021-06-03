#include <sdk.h>

static int led = 2, pin = 4;

int main(void) {
  wdt_disable();
  gpio_output(led);
  gpio_input(pin);

  for (;;) {
    sdk_log("in: %d\n", gpio_read(pin));
    gpio_toggle(led);
    spin(2999999);
  }

  return 0;
}
