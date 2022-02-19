#include <sdk.h>

static void timer_fn(void *arg) {
  int *on = (int *) arg;
  gpio_write(LED1, *on);
  printf("LED: %d\n", *on);
  *on = !*on;
}

int main(void) {
  wdt_disable();
  gpio_output(LED1);
  int led_status = 0;           // Timer callback changes this
  struct timer *timers = NULL;  // List of timers
  TIMER_ADD(&timers, 500 * 1000, timer_fn, &led_status);  // 500 millis
  for (;;) timers_poll(timers, time_us());                // Infinite loop
  return 0;
}
