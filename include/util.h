// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

int sdk_ram_used(void);
int sdk_ram_free(void);

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define NOPS_PER_MS 99999999
#else
#define NOPS_PER_MS 26643
#endif

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

static inline void delay_us(volatile unsigned long us) {
  spin(us * NOPS_PER_MS / 1000);
}

static inline void delay_ms(volatile unsigned long ms) {
  spin(ms * NOPS_PER_MS);
}
