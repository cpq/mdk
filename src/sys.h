// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

int sdk_ram_used(void);
int sdk_ram_free(void);

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
}

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <time.h>
#define NOPS_PER_MS 99999999
static inline unsigned long time_us(void) {
  return (unsigned long) time(NULL) * 1000;
}
#else
#define TIMG0_REG REG(0x6001f000)  // Timer group 0
#define TIMG1_REG REG(0x60020000)  // Timer group 1
#define NOPS_PER_MS 26643
static inline unsigned long time_us(void) {
  TIMG0_REG[3] = BIT(31);  // TRM 8.3.1, request to fetch timer value
  spin(1);                 // Is there a better way to wait?
  return TIMG0_REG[1];     // Read low bits, ignore high - don't bother
}

static inline void delay_us2(unsigned long us) {
  unsigned long until = time_us() + us;  // Handler counter wrap
  while (time_us() < until) spin(1);
}
#endif

static inline void delay_us(volatile unsigned long us) {
  spin(us * NOPS_PER_MS / 1000);
}

static inline void delay_ms(volatile unsigned long ms) {
  spin(ms * NOPS_PER_MS);
  // delay_us(ms * 1000);
}

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
