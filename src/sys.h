// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

int sdk_ram_used(void);
int sdk_ram_free(void);

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

#if defined(ESP32C3)

static inline unsigned long time_us(void) {
  REG(C3_TIMERGROUP0)[3] = BIT(31);  // TRM 8.3.1, request to fetch timer value
  spin(1);                           // Is there a better way to wait?
  return REG(C3_TIMERGROUP0)[1];     // Read low bits, ignore high
}

static inline void delay_us(unsigned long us) {
  unsigned long until = time_us() + us;  // Handler counter wrap
  while (time_us() < until) spin(1);
}

static inline void delay_ms(unsigned long ms) {
  delay_us(ms * 1000);
}

#elif defined(ESP32)

#define NOPS_PER_MS 26643
static inline void delay_us(volatile unsigned long us) {
  spin(us * NOPS_PER_MS / 1000);
}

static inline void delay_ms(volatile unsigned long ms) {
  spin(ms * NOPS_PER_MS);
}

#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
static inline unsigned long time_us(void) {
  return (unsigned long) time(NULL) * 1000000;
}

#else
#error "Ouch"
#endif

// Linked list management macros
#define LIST_ADD(head_, elem_) \
  do {                         \
    (elem_)->next = (*head_);  \
    *(head_) = (elem_);        \
  } while (0)

#define LIST_DEL(type_, head_, elem_)      \
  do {                                     \
    type_ **h = head_;                     \
    while (*h != (elem_)) h = &(*h)->next; \
    *h = (elem_)->next;                    \
  } while (0)
