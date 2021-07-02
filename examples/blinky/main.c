#include <sdk.h>

static int led_pin = LED1;  // To override: make EXTRA_CFLAGS=-DLED1=5
static int led_state = 0;

int main(void) {
  wdt_disable();
  gpio_output(led_pin);

  for (;;) {
    gpio_write(led_pin, led_state);  // Blink an LED
    sdk_log("%d\n", led_state);      // Print LED status
    led_state ^= 1;                  // Toggle state
    delay_ms(500);                   // Delay a bit
  }

  return 0;
}
