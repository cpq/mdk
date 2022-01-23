// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include "soc.h"

enum { GPIO_OUT_EN = 8, GPIO_OUT_FUNC = 341, GPIO_IN_FUNC = 85 };

static inline void gpio_output_enable(int pin, bool enable) {
  REG(C3_GPIO)[GPIO_OUT_EN] &= ~BIT(pin);
  REG(C3_GPIO)[GPIO_OUT_EN] |= (enable ? 1U : 0U) << pin;
}

static inline void gpio_output(int pin) {
  REG(C3_GPIO)[GPIO_OUT_FUNC + pin] = BIT(9) | 128;  // Simple out, TRM 5.5.3
  gpio_output_enable(pin, 1);
}

static inline void gpio_write(int pin, bool value) {
  REG(C3_GPIO)[1] &= ~BIT(pin);                 // Clear first
  REG(C3_GPIO)[1] |= (value ? 1U : 0U) << pin;  // Then set
}

static inline void gpio_toggle(int pin) {
  REG(C3_GPIO)[1] ^= BIT(pin);
}

static inline void gpio_input(int pin) {
  gpio_output_enable(pin, 0);                 // Disable output
  REG(C3_IO_MUX)[1 + pin] = BIT(9) | BIT(8);  // Enable pull-up
}

static inline bool gpio_read(int pin) {
  return REG(C3_GPIO)[15] & BIT(pin) ? 1 : 0;
}
