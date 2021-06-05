// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BIT(x) (1UL << (x))
#define REG(x) ((volatile unsigned long *) (x))

#ifndef LED1
#define LED1 2  // Default LED pin
#endif

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

void sdk_log(char *fmt, ...);
void sdk_vlog(char *fmt, va_list);
int sdk_ram_used(void);
int sdk_ram_free(void);

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
// Allow to build "firmwares" as unix binaries by emulating hardware API
#include "unix.h"
#else
#include "gpio.h"
#include "uart.h"
#include "wdt.h"
#endif

#include "cnip.h"
#include "spi.h"
