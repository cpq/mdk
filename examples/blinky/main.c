#include <sdk.h>

static int led_pin = 2;

int main(void) {
  // Disable watchdogs. Otherwise they will keep rebooting us
  *RTC_CNTL_WDTCONFIG0_REG &= BIT(31);  // Disable RTC WDT
  *TIMG0_T0_WDTCONFIG0_REG &= BIT(31);  // Disable task WDT

  gpio_output(led_pin);
  for (;;) {
    sdk_log("hi\n");       // Print something to the serial console
    gpio_toggle(led_pin);  // Blink an LED
    spin(2999999);         // Delay a bit
  }

  return 0;
}
