#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define BIT(x) (1UL << (x))
#define REG(x) ((volatile uint32_t *) (x))
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
  while (count) count--;
}

void sdk_log(char *fmt, ...);
void sdk_vlog(char *fmt, va_list);
