#pragma once

#if defined(ESP32C3)
#define GPIO_FUNC_OUT_SEL_CFG_REG REG(0x60004554)
#define GPIO_OUT_REG REG(0x60004004)
#define GPIO_IN_REG REG(0x6000403c)
#define GPIO_ENABLE_REG REG(0x60004020)

static inline void gpio_output_enable(int pin, bool enable) {
  GPIO_ENABLE_REG[0] &= ~BIT(pin);
  GPIO_ENABLE_REG[0] |= (enable ? 1U : 0U) << pin;
}

static inline void gpio_output(int pin) {
  GPIO_FUNC_OUT_SEL_CFG_REG[pin] = 128;  // Simple output, TRM 5.5.3
  gpio_output_enable(pin, 1);
}

static inline void gpio_write(int pin, bool value) {
  GPIO_OUT_REG[0] &= ~BIT(pin);                 // Clear first
  GPIO_OUT_REG[0] |= (value ? 1U : 0U) << pin;  // Then set
}

static inline void gpio_toggle(int pin) {
  GPIO_OUT_REG[0] ^= BIT(pin);
}

static inline void gpio_input(int pin) {
  gpio_output_enable(pin, 0);  // Disable output
}

static inline bool gpio_read(int pin) {
  return GPIO_IN_REG[0] & BIT(pin) ? 1 : 0;
}

#elif defined(ESP32)
#define GPIO_FUNC_OUT_SEL_CFG_REG REG(0X3ff44530)  // Pins 0-39
#define GPIO_FUNC_IN_SEL_CFG_REG REG(0X3ff44130)   // Pins 0-39
#define GPIO_OUT_REG REG(0X3ff44004)               // Pins 0-31
#define GPIO_IN_REG REG(0x3FF4403C)                // Pins 0-31
#define GPIO_ENABLE_REG REG(0X3ff44020)            // Pins 0-31
#define GPIO_OUT1_REG REG(0X3ff44010)              // Pins 32-39
#define GPIO_IN1_REG REG(0X3ff44040)               // Pins 32-39
#define GPIO_ENABLE1_REG REG(0X3ff4402c)           // Pins 32-39

static inline void gpio_output_enable(int pin, bool enable) {
  volatile unsigned long *r = GPIO_ENABLE_REG;
  if (pin > 31) pin -= 31, r = GPIO_ENABLE1_REG;
  r[0] &= ~BIT(pin);
  r[0] |= (enable ? 1U : 0U) << pin;
}

static inline void gpio_output(int pin) {
  GPIO_FUNC_OUT_SEL_CFG_REG[pin] = 256;  // Simple output, TRM 4.3.3
  gpio_output_enable(pin, 1);
}

static inline void gpio_write(int pin, bool value) {
  volatile unsigned long *r = GPIO_OUT_REG;
  if (pin > 31) pin -= 31, r = GPIO_OUT1_REG;
  r[0] &= ~BIT(pin);               // Clear first
  r[0] |= (value ? 1U : 0U) << pin;  // Then set
}

static inline void gpio_toggle(int pin) {
  volatile unsigned long *r = GPIO_OUT_REG;
  if (pin > 31) pin -= 31, r = GPIO_OUT1_REG;
  r[0] ^= BIT(pin);
}

static inline void gpio_input(int pin) {
  // Index lookup table for IO_MUX_GPIOx_REG, TRM 4.12
  unsigned char map[40] = {17, 34, 16, 33, 18, 27, 24, 25, 26, 21,  // 0-9
                           22, 23, 13, 14, 12, 15, 19, 20, 28, 29,  // 10-19
                           30, 31, 32, 35, 36, 9,  10, 11, 0,  0,   // 20-29
                           0,  0,  7,  8,  5,  6,  1,  2,  3,  4};  // 30-39
  volatile unsigned long *mux = REG(0X3ff49000);
  if (pin < 0 || pin > (int) sizeof(map) || map[pin] == 0) return;
  gpio_output_enable(pin, 0);  // Disable output
  mux[map[pin]] |= BIT(9);     // Enable input
}

static inline bool gpio_read(int pin) {
  volatile unsigned long *r = GPIO_IN_REG;
  if (pin > 31) pin -= 31, r = GPIO_IN1_REG;
  return r[0] & BIT(pin) ? 1 : 0;
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <stdio.h>
#include <unistd.h>
static inline void gpio_write(int pin, bool value) {
  printf("%s %d %d\n", __func__, pin, value);
}

static inline bool gpio_read(int pin) {
  printf("%s %d\n", __func__, pin);
  return 0;
}

static inline void gpio_toggle(int pin) {
  printf("%s %d\n", __func__, pin);
}

static inline void gpio_input(int pin) {
  printf("%s %d\n", __func__, pin);
}

static inline void gpio_output(int pin) {
  printf("%s %d\n", __func__, pin);
}
#else
#error "Ouch"
#endif
