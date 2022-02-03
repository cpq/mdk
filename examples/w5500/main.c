#include <sdk.h>
#include "drivers/w5500.h"

static int led_pin = LED1;  // To override: make EXTRA_CFLAGS=-DLED1=5
static int led_state = 0;

int main(void) {
  wdt_disable();
  gpio_output(led_pin);

  struct spi spi = {.mosi = 23, .miso = 19, .clk = 18, .cs = {5, -1, -1}};
  spi_init(&spi);

  struct w5500 wiz = {.spi = &spi, .cs = 5, .mac = {0xaa, 0xbb, 0xcc, 1, 2, 3}};
  w5500_init(&wiz);

  for (;;) {
    gpio_write(led_pin, led_state);  // Blink an LED
    sdk_log("%d\n", led_state);      // Print LED status
    led_state ^= 1;                  // Toggle state
    delay_ms(500);                   // Delay a bit
  }

  return 0;
}
