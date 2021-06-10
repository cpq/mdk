#include <sdk.h>

struct ppp {
  int state;  // dead, setup, auth, run, close
};

static void lcp(void) {
  uint8_t pkt[] = {1, 2, 3, 4, 5};
  for (size_t i = 0; i < sizeof(pkt); i++) uart_tx(pkt[i]);
}

int main(void) {
  wdt_disable();  // Shut up our friend for now

  gpio_output(LED1);
  for (;;) spin(999999), gpio_toggle(LED1), lcp();

  return 0;  // Unreached
}
