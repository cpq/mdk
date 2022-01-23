// Copyright (c) 2021 Cesanta
// All rights reserved

#include "sdk.h"

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
