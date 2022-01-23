// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

static inline void gpio_write(int pin, bool value) {
  (void) pin, (void) value;
  // printf("%s %d %d\n", __func__, pin, value);
}

static inline bool gpio_read(int pin) {
  // printf("%s %d\n", __func__, pin);
  (void) pin;
  return 0;
}

static inline void gpio_toggle(int pin) {
  (void) pin;
  // printf("%s %d\n", __func__, pin);
}

static inline void gpio_input(int pin) {
  (void) pin;
  // printf("%s %d\n", __func__, pin);
}

static inline void gpio_output(int pin) {
  (void) pin;
  // printf("%s %d\n", __func__, pin);
}
