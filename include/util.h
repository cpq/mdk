// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

int sdk_ram_used(void);
int sdk_ram_free(void);

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}
