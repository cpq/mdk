// Copyright (c) 2022 Cesanta
// All rights reserved

#pragma once

#define BIT(x) ((uint32_t) 1U << (x))
#define REG(x) ((volatile uint32_t *) (x))

// Perform `count` "NOP" operations
static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

#ifndef LED1
#define LED1 1  // Default LED pin
#endif

#ifndef BTN1
#define BTN1 9  // Default user button pin
#endif
