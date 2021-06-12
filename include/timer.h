// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdint.h>

struct timer {
  uint64_t period;       // Timer period in micros
  uint64_t expire;       // Expiration timestamp in micros
  void (*fn)(void *);    // Function to call
  void *arg;             // Function argument
  struct timer *next;    // Linkage
};

#define TIMER_ADD(head_, p_, fn_, arg_)                                    \
  do {                                                                     \
    static struct timer t_ = {.period = (p_), .fn = (fn_), .arg = (arg_)}; \
    LIST_ADD(head_, &t_);                                                  \
  } while (0)

#define TIMER_DEL(head_, t_) LIST_DELETE(struct timer, head_, t_)

static inline void timers_poll(struct timer *head, uint64_t now) {
  struct timer *t, *tmp;
  for (t = head; t != NULL; t = tmp) {
    tmp = t->next;
    if (t->expire == 0) t->expire = now + t->period;
    if (t->expire > now) continue;
    t->fn(t->arg);
    // Try to tick timers with the given period as accurate as possible,
    // even if this polling function is called with some random period.
    t->expire =
        now - t->expire > t->period ? now + t->period : t->expire + t->period;
  }
}
