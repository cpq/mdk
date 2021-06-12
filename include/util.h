// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

int sdk_ram_used(void);
int sdk_ram_free(void);

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
#define NOPS_PER_MS 99999999
static inline uint64_t time_us(void) {
  return time(NULL) * 1000;
}
#else
#define TIMG0_REG REG(0x6001f000)  // Timer group 0
#define TIMG1_REG REG(0x60020000)  // Timer group 1
#define NOPS_PER_MS 26643
static inline uint64_t time_us(void) {
  TIMG0_REG[3] = 0;  // TRM 8.3.1. Write to TIMG_T0UPDATE_REG, then read value
  // We >> 4 cause 1 micro is 16 timer ticks
  return (((uint64_t) TIMG0_REG[2]) << 28) | (TIMG0_REG[1] >> 4);
}
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
