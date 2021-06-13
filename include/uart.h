// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

// TODO(cpq): implement properly!
static inline unsigned long uart_tx_pin(int pin, int baud, uint8_t c) {
  unsigned long s = time_us(), t = (unsigned long) (1000000 / baud), u = s;
  gpio_write(pin, 0);
  for (u += t; time_us() < u;) spin(1);
  for (int i = 0; i < 8; i++) {
    gpio_write(pin, c & (1U << i));
    for (u += t; time_us() < u;) spin(1);
  }
  gpio_write(pin, 1);
  for (u += t; time_us() < u;) spin(1);
  return time_us() - s;
}

// Polling read, very unreliable. Interrupts should be used
// Return 1 if a character was read, 0 otherwise
static inline int uart_rx_pin(int pin, int baud, uint8_t *c) {
  unsigned long s = time_us(), t = (unsigned long) (1000000 / baud);
  if (gpio_read(pin)) return -1;
  for (s += t * 3 / 2; time_us() < s;) spin(1);
  *c = 0;
  for (int i = 0; i < 8; i++) {
    *c |= (uint8_t)(gpio_read(pin) << i);
    for (s += t; time_us() < s;) spin(1);
  }
  for (s += t / 2; time_us() < s;) spin(1);
  // Wait here for the next start bit
  for (s += t * 50; gpio_read(pin) > 0 && time_us() < s;) spin(1);
  return 0;
}

#if defined(ESP32C3) || defined(ESP32)
static inline int uart_rx(unsigned char *c) {
  extern int uart_rx_one_char(unsigned char *);
  return uart_rx_one_char(c);
}

static inline int uart_tx(unsigned char byte) {
  extern int uart_tx_one_char(unsigned char);
  return uart_tx_one_char(byte);
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
static inline int uart_tx(uint8_t ch) {
  while (write(1, &ch, 1) != 1) (void) 0;
  return 0;
}
static inline int uart_rx(uint8_t *ch) {
  return read(0, ch, 1) == 1 ? 0 : -1;
}
static inline int uart_tx_one_char(uint8_t ch) {
  return write(1, &ch, 1) > 0 ? 0 : -1;
}
#else
#error "Ouch"
#endif
