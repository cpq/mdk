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

#define BIT(x) ((uint32_t) 1 << (x))
#define REG(x) ((volatile uint32_t *) (x))

int sdk_ram_used(void);
int sdk_ram_free(void);

static inline void spin(volatile unsigned long count) {
  while (count--) asm volatile("nop");
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

#include "gpio.h"
#include "log.h"
#include "soc.h"
#include "spi.h"
#include "tcpip.h"
#include "timer.h"
#include "uart.h"
#include "ws2812.h"

#ifndef LED1
#define LED1 1  // Default LED pin
#endif

#ifndef BTN1
#define BTN1 9  // Default user button pin
#endif
