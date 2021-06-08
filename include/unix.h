#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int uart_tx(uint8_t ch);
int uart_rx(uint8_t *ch);

static inline void gpio_write(int pin, bool value) {
  printf("%s %d %d\n", __func__, pin, value);
}

static inline bool gpio_read(int pin) {
  printf("%s %d\n", __func__, pin);
  return 0;
}

static inline void gpio_toggle(int pin) {
  printf("%s %d\n", __func__, pin);
}

static inline void gpio_input(int pin) {
  printf("%s %d\n", __func__, pin);
}

static inline void gpio_output(int pin) {
  printf("%s %d\n", __func__, pin);
}

static inline void wdt_disable(void) {
}
