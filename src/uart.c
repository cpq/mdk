// Copyright (c) 2021 Cesanta
// All rights reserved

#include "uart.h"
#include "gpio.h"
#include "soc.h"
#include "sys.h"
#include "timer.h"

// Software UART send
void uart_tx_pin(int pin, int baud, uint8_t c) {
  unsigned long s = time_us(), t = (unsigned long) (100000000 / baud);
  gpio_write(pin, 0);
  for (s += t / 100; time_us() < s;) spin(1);
  for (int i = 0; i < 8; i++) {
    gpio_write(pin, c & (1U << i));
    for (s += t / 100; time_us() < s;) spin(1);
  }
  gpio_write(pin, 1);
  for (s += t / 100; time_us() < s;) spin(1);
}

// Software UART receive. Polling, very unreliable. Interrupts should be used
// Return true if a character was read, false otherwise
bool uart_rx_pin(int pin, int baud, uint8_t *c) {
  unsigned long s = time_us(), t = (unsigned long) (100000000 / baud);
  if (gpio_read(pin)) return false;
  for (s += t * 11 / 800; time_us() < s;) spin(1);
  *c = 0;
  for (int i = 0; i < 8; i++) {
    // gpio_write(1, 1), gpio_write(1, 0); // Pulse to see on a scope
    *c |= (uint8_t)(gpio_read(pin) << i);
    for (s += t / 100; time_us() < s;) spin(1);
  }
  // Wait here for the next start bit
  for (s += t; gpio_read(pin) > 0 && time_us() < s;) spin(1);
  return true;
}

#if defined(ESP32C3)
void uart_init(int no, int tx, int rx, int baud) {
  enum { FIFO = 0, CLK_DIV = 5, STATUS = 7, CONF0, CONF1, CLK_CONF = 30 };
  volatile uint32_t *uart = no ? REG(C3_UART1) : REG(C3_UART);
  uint32_t div = (uint32_t)(4000U * 1000000 / (uint32_t) baud);
  uart[CLK_DIV] = div / 100;                        // Integral part
  uart[CLK_DIV] |= (16 * (div % 100) / 100) << 20;  // Fractional part
  uart[CONF0] &= ~BIT(28);                          // Clear UART_MEM_CLK_EN
  uart[CONF0] |= BIT(26);                           // Set UART_ERR_WR_MASK
  uart[CONF1] = 1;                                  // RXFULL -> 1, TXEMPTY -> 0
  uart[CLK_CONF] = BIT(25) | BIT(24) | BIT(22) | BIT(21) | BIT(20);  // Use APB

  // Set TX pin
  REG(C3_GPIO)[GPIO_OUT_EN] |= BIT(tx);
  REG(C3_GPIO)[GPIO_OUT_FUNC + tx] &= ~255U;
  REG(C3_GPIO)[GPIO_OUT_FUNC + tx] |= no ? 9 : 6;

  // Set RX pin
  REG(C3_GPIO)[GPIO_OUT_EN] &= ~BIT(rx);                 // Disable output
  REG(C3_IO_MUX)[1 + rx] = BIT(8) | BIT(9) | (1U << 9);  // input, PU, GPIO
  REG(C3_GPIO)[GPIO_IN_FUNC + (no ? 9 : 6)] = BIT(6) | (uint8_t) rx;
}

unsigned long uart_status(int no) {
  volatile uint32_t *uart = no ? REG(C3_UART1) : REG(C3_UART);
  return uart[7];
}

#define UART_TX_FIFO_LEN(uart) ((uart[7] >> 16) & 127)
#define UART_RX_FIFO_LEN(uart) ((uart[7]) & 127)

bool uart_read(int no, uint8_t *c) {
  volatile uint32_t *uart = no ? REG(C3_UART1) : REG(C3_UART);
  if (UART_RX_FIFO_LEN(uart) == 0) return false;
  *c = uart[0] & 255;
  return true;
}

void uart_write(int no, uint8_t c) {
  volatile uint32_t *uart = no ? REG(C3_UART1) : REG(C3_UART);
  while (UART_TX_FIFO_LEN(uart) > 100) delay_us(1);
  uart[0] = c;
}
#elif defined(ESP32)
void uart_write(int no, uint8_t c) {
  extern int uart_tx_one_char(int);
  if (no == 0) (void) uart_tx_one_char(c);
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)

#if !defined(UART1)
#define UART1 "/dev/cu.usbserial-1420"
#endif

static int s_uart1_fd = -1;

static bool fd_ready(int fd) {
  struct timeval tv = {.tv_sec = 0, .tv_usec = 1};
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  return select(fd + 1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &fds);
}

static int open_serial(const char *name, int baud) {
  int fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  // int fd = open(name, O_RDWR | O_NONBLOCK);
  struct termios tio;
  if (fd >= 0 && tcgetattr(fd, &tio) == 0) {
    tio.c_ispeed = tio.c_ospeed = (speed_t) baud;
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = tio.c_oflag = tio.c_iflag = 0;
    tcsetattr(fd, TCSANOW, &tio);
  }
  printf("Opened %s @ %d, fd %d errno %d\n", name, baud, fd, errno);
  return fd;
}

bool uart_read(int no, uint8_t *c) {
  int fd = no == 0 ? 0 : s_uart1_fd;
  if (!fd_ready(fd)) return false;
  return read(fd, c, 1) == 1;
}

void uart_write(int no, uint8_t c) {
  int fd = no == 0 ? 1 : s_uart1_fd;
  write(fd, &c, 1);
}

void uart_init(int no, int tx, int rx, int baud) {
  if (no != 0 && s_uart1_fd == -1) s_uart1_fd = open_serial(UART1, baud);
  (void) rx;
  (void) tx;
}
#else
#error "Ouch"
#endif
