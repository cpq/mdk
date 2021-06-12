#include <sdk.h>

static void timer_fn(void *arg) {
  gpio_toggle(LED1);
  (void) arg;
}

int main(void) {
  wdt_disable();
  gpio_output(LED1);

  struct timer *timers = NULL;                     // List of timers
  TIMER_ADD(&timers, 500 * 1000, timer_fn, NULL);  // 500 millis

  for (;;) timers_poll(timers, time_us());

  return 0;
}
