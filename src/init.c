// Copyright (c) 2021 Cesanta
// All rights reserved

#include "sdk.h"

extern int main(void);

static char *s_heap_start;  // Heap start. Set up by heap_init()
static char *s_heap_end;    // Heap end
static char *s_brk;         // Current heap usage marker

// Initialise newlib malloc. It expects us to implement _sbrk() call,
// which should return a pointer to the requested memory.
// Our `heap_init` function sets up an available memory region.
//
//   s_heap_start                s_brk               s_heap_end
//      |--------------------------|----------------------|
//      |      (used memory)       |    (free memory)     |

int sdk_ram_used(void) {
  return (int) (s_brk - s_heap_start);
}

int sdk_ram_free(void) {
  return (int) (s_heap_end - s_brk);
}

static void heap_init(void *start, void *end) {
  s_heap_start = s_brk = start, s_heap_end = end;
}

void *sbrk(int diff) {
  char *old = s_brk;
  if (&s_brk[diff] > s_heap_end) return NULL;
  s_brk += diff;
  return old;
}

static void clock_init(void) {
#if defined(ESP32C3)
  // TRM 6.2.4.1
  REG(C3_SYSTEM)[2] &= ~3U;
  REG(C3_SYSTEM)[2] |= BIT(0) | BIT(2);
  REG(C3_SYSTEM)[22] = BIT(19) | (40U << 12) | BIT(10);
  // REG(C3_RTCCNTL)[47] = 0; // RTC_APB_FREQ_REG -> freq >> 12
  ((void (*)(int)) 0x40000588)(160);  // ets_update_cpu_frequency(160)

  // Configure system clock timer, TRM 8.3.1, 8.9
  REG(C3_TIMERGROUP0)[1] = REG(C3_TIMERGROUP0)[2] = 0UL;  // Reset LO and HI
  REG(C3_TIMERGROUP0)[8] = 0;                             // Trigger reload
  REG(C3_TIMERGROUP0)[0] = (83U << 13) | BIT(12) | BIT(29) | BIT(30) | BIT(31);
#endif
}

// Initialise memory and other low level stuff, and call main()
void _start(void) {
#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
  char _sbss, _ebss, _end, _eram;
#else
  extern char _sbss, _ebss, _end, _eram;
#endif
  for (char *p = &_sbss; p < &_ebss;) {
    *p++ = '\0';
  }
  heap_init(&_end, &_eram);
  clock_init();
  main();
}
