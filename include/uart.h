// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include "util.h"

// TODO(lsm): implement properly!
static inline void uart_tx_pin(int pin, int baud, uint8_t c) {
  volatile unsigned long t = (unsigned long) (1000000 / baud);
  gpio_write(pin, 0);
  delay_us(t);
  for (int i = 0; i < 8; i++) {
    gpio_write(pin, c & (1U << i));
    delay_us(t);
  }
  gpio_write(pin, 1);
  delay_us(t);
}

// Polling read, very unreliable. Interrupts should be used
// Return 1 if a character was read, 0 otherwise
static inline int uart_rx_pin(int pin, int baud, uint8_t *c) {
  volatile unsigned long t = (unsigned long) (1000000 / baud);
  if (gpio_read(pin)) return 0;
  delay_us(t * 3 / 2);
  *c = 0;
  for (int i = 0; i < 8; i++) {
    *c |= (uint8_t)(gpio_read(pin) << i);
    delay_us(t);
  }
  delay_us(t / 3);
  // Wait here for the next start bit
  for (unsigned long i = 0; gpio_read(pin) > 0 && i < t * 50;) i++;
  return 1;
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
int uart_tx(uint8_t ch);
int uart_rx(uint8_t *ch);
#else
#error "Ouch"
#endif
