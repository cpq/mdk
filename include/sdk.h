#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define BIT(x) (1UL << (x))
#define REG(x) ((volatile unsigned long *) (x))

#define RTC_CNTL_WDTCONFIG0_REG REG(0X3ff4808c)
#define TIMG0_T0_WDTCONFIG0_REG REG(0X3ff5f048)
#define DPORT_PRO_CACHE_CTRL1_REG REG(0x3ff00044)
#define GPIO_FUNC_OUT_SEL REG(0X3ff44530)
#define GPIO_OUT_REG REG(0X3ff44004)
#define GPIO_ENABLE_REG REG(0X3ff44020)

static inline void gpio_output(int pin) {
  GPIO_FUNC_OUT_SEL[pin] = 256;  // Configure GPIO as a simple output, TRM 4.3.3
  GPIO_ENABLE_REG[0] |= BIT(pin);
}

static inline void gpio_toggle(int pin) {
  GPIO_OUT_REG[0] ^= BIT(pin);
}

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

void sdk_log(char *fmt, ...);
void sdk_vlog(char *fmt, va_list);
int sdk_ram_used(void);
int sdk_ram_free(void);
