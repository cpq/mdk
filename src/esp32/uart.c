// Copyright (c) 2021 Cesanta
// All rights reserved

#include "sdk.h"

void uart_write(int no, uint8_t c) {
  extern int uart_tx_one_char(int);
  if (no == 0) (void) uart_tx_one_char(c);
}
