// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

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
