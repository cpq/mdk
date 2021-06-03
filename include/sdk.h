// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define BIT(x) (1UL << (x))
#define REG(x) ((volatile unsigned long *) (x))

#ifndef LED1
#define LED1 2  // Default LED pin
#endif

#include "gpio.h"
#include "spi.h"
#include "wdt.h"

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

void sdk_log(char *fmt, ...);
void sdk_vlog(char *fmt, va_list);
int sdk_ram_used(void);
int sdk_ram_free(void);
