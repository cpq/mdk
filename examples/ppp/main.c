#include <sdk.h>

int main(void) {
  wdt_disable();  // Shut up our friend for now

  gpio_output(LED1);
  for (;;) spin(999999), gpio_toggle(LED1);

  return 0;  // Unreached
}
