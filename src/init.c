// Copyright (c) 2021 Cesanta
// All rights reserved

#include "sdk.h"

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
#else
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

void __assert_func(const char *a, int b, const char *c, const char *d) {
  sdk_log("ASSERT %s %d %s %s\n", a, b, c, d);
  gpio_output(LED1);
  for (;;) spin(199999), gpio_toggle(LED1);
}

// Initialise memory and other low level stuff, and call main()
void start(void) {
  extern char _sbss, _ebss, _end, _eram;
  for (char *p = &_sbss; p < &_ebss;) *p++ = '\0';
  heap_init(&_end, &_eram);
  soc_init();
  main();
}
#endif
