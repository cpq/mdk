#include <sdk.h>

extern int main(void);

static char *s_heap_start;  // Heap start. Set up by sdk_heap_init()
static char *s_heap_end;    // Heap end
static char *s_brk;         // Current heap usage marker

// Initialise newlib malloc. It expects us to implement _sbrk() call,
// which should return a pointer to the requested memory.
// Our `sdk_heap_init` function sets up an available memory region.
//
//   s_heap_start                s_brk               s_heap_end
//      |--------------------------|----------------------|
//      |      (used memory)       |    (free memory)     |

int sdk_ram_used(void) {
  return s_brk - s_heap_start;
}

int sdk_ram_free(void) {
  return s_heap_end - s_brk;
}

static void sdk_heap_init(void *start, void *end) {
  s_heap_start = s_brk = start, s_heap_end = end;
}

void *sbrk(int diff) {
  char *old = s_brk;
  if (&s_brk[diff] > s_heap_end) return NULL;
  s_brk += diff;
  return old;
}

void __assert_func(const char *a, int b, const char *c, const char *d) {
  gpio_output(2);
  for (;;) spin(199999), gpio_toggle(2);
  (void) a;
  (void) b;
  (void) c;
  (void) d;
}

// Initialise memory and other low level stuff, and call main()
void startup(void) {
  extern char _sbss, _ebss;
  memset(&_sbss, 0, (size_t)(&_ebss - &_sbss));

  extern char _end, _eram;
  sdk_heap_init(&_end, &_eram);

  main();
}
