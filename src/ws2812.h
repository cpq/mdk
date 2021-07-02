// Copyright (c) Charles Lohr
// All rights reserved

#pragma once

static inline void ws2812_show(int pin, const uint8_t *buf, size_t len) {
  unsigned long delays[2] = {2, 6};
  for (size_t i = 0; i < len; i++) {
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
      int i1 = buf[i] & mask ? 1 : 0, i2 = i1 ^ 1;  // This takes some cycles
      gpio_write(pin, 1);
      spin(delays[i1]);
      gpio_write(pin, 0);
      spin(delays[i2]);
    }
  }
}
