// Copyright (c) 2021 Cesanta
// All rights reserved

#include "mdk.h"

void gpio_output_enable(int pin, bool enable) {
  REG(C3_GPIO)[GPIO_OUT_EN] &= ~BIT(pin);
  REG(C3_GPIO)[GPIO_OUT_EN] |= (enable ? 1U : 0U) << pin;
}

void gpio_output(int pin) {
  REG(C3_GPIO)[GPIO_OUT_FUNC + pin] = BIT(9) | 128;  // Simple out, TRM 5.5.3
  gpio_output_enable(pin, 1);
}

void gpio_write(int pin, bool value) {
  REG(C3_GPIO)[1] &= ~BIT(pin);                 // Clear first
  REG(C3_GPIO)[1] |= (value ? 1U : 0U) << pin;  // Then set
}

void gpio_toggle(int pin) {
  REG(C3_GPIO)[1] ^= BIT(pin);
}

void gpio_input(int pin) {
  gpio_output_enable(pin, 0);                 // Disable output
  REG(C3_IO_MUX)[1 + pin] = BIT(9) | BIT(8);  // Enable pull-up
}

bool gpio_read(int pin) {
  return REG(C3_GPIO)[15] & BIT(pin) ? 1 : 0;
}
