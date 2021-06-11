// Copyright (c) 2021 Cesanta
// All rights reserved

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
  return (int) (s_brk - s_heap_start);
}

int sdk_ram_free(void) {
  return (int) (s_heap_end - s_brk);
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
  sdk_log("ASSERT %s %d %s %s\n", a, b, c, d);
  gpio_output(LED1);
  for (;;) spin(199999), gpio_toggle(LED1);
}

static void clock_init(void) {
#if defined(ESP32C3)
  // TRM 6.2.4.1
#define SYSTEM_SYSCLK_CONF_REG REG(0x600c0058)
#define SYSTEM_CPU_PER_CONF_REG REG(0x600c0008)
  SYSTEM_SYSCLK_CONF_REG[0] &= 3U << 10;
  SYSTEM_SYSCLK_CONF_REG[0] |= 1U << 10;
  SYSTEM_CPU_PER_CONF_REG[0] &= 3U;
  SYSTEM_CPU_PER_CONF_REG[0] |= BIT(2) | BIT(0);
#endif
}

// Initialise memory and other low level stuff, and call main()
void startup(void) {
  extern char _sbss, _ebss;
  for (char *p = &_sbss; p < &_ebss;) *p++ = '\0';
  extern char _end, _eram;
  sdk_heap_init(&_end, &_eram);
  clock_init();
  main();
}

#if defined(__unix) || defined(__unix__) || defined(__APPLE__)
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

char _sbss, _ebss, _end, _eram;
static int s_uart = -1;
static const char *s_serial_port = "/dev/ptyp3";

static int open_serial(const char *name, int baud) {
  int fd = open(name, O_RDWR | O_NONBLOCK);
  struct termios tio;
  if (fd >= 0 && tcgetattr(fd, &tio) == 0) {
    tio.c_ispeed = tio.c_ospeed = (speed_t) baud;
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = tio.c_oflag = tio.c_iflag = 0;
    tcsetattr(fd, TCSANOW, &tio);
  }
  printf("Opened %s @ %d, fd %d (%s)\n", name, baud, fd, strerror(errno));
  return fd;
}
int uart_tx(uint8_t ch) {
  if (s_uart < 0) s_uart = open_serial(s_serial_port, 115200);
  while (write(s_uart, &ch, 1) != 1) (void) 0;
  return 0;
}
int uart_rx(uint8_t *ch) {
  if (s_uart < 0) s_uart = open_serial(s_serial_port, 115200);
  return read(s_uart, ch, 1) == 1 ? 0 : -1;
}
int uart_tx_one_char(uint8_t ch) {
  // return uart_tx(ch);
  return write(1, &ch, 1) > 0 ? 0 : -1;
}
#endif
