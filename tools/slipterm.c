// Copyright (c) 2021 Charles Lohr
// All rights reserved

#define SLIPTERM_VERSION "0.01"

// GENERAL NOTEs/TODO:
//   1) Should we use ctrl+] like IDF?
//   2) Actually test.

#include <errno.h>
#include <fcntl.h>
#include <pcap.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "slip.h"  // SLIP state machine logic

static int s_signo;

void signal_handler(int signo) {
  s_signo = signo;
}

static int fail(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

static char *hexdump(const void *buf, size_t len, char *dst, size_t dlen) {
  const unsigned char *p = (const unsigned char *) buf;
  size_t i, idx, n = 0, ofs = 0;
  char ascii[17] = "";
  if (dst == NULL) return dst;
  memset(dst, ' ', dlen);
  for (i = 0; i < len; i++) {
    idx = i % 16;
    if (idx == 0) {
      if (i > 0 && dlen > n)
        n += (size_t) snprintf(dst + n, dlen - n, "  %s\n", ascii);
      if (dlen > n)
        n += (size_t) snprintf(dst + n, dlen - n, "%04x ", (int) (i + ofs));
    }
    if (dlen < n) break;
    n += (size_t) snprintf(dst + n, dlen - n, " %02x", p[i]);
    ascii[idx] = (char) (p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i]);
    ascii[idx + 1] = '\0';
  }
  while (i++ % 16) {
    if (n < dlen) n += (size_t) snprintf(dst + n, dlen - n, "%s", "   ");
  }
  if (n < dlen) n += (size_t) snprintf(dst + n, dlen - n, "  %s\n", ascii);
  if (n > dlen - 1) n = dlen - 1;
  dst[n] = '\0';
  return dst;
}

static int open_serial(const char *name, int baud) {
  int fd = open(name, O_RDWR);
  struct termios tio;
  if (fd >= 0 && tcgetattr(fd, &tio) == 0) {
    tio.c_ispeed = tio.c_ospeed = baud;  // Input speed = output speed = baud
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_lflag = tio.c_oflag = tio.c_iflag = 0;
    tcsetattr(fd, TCSANOW, &tio);
  }
  printf("Opened %s @ %d\n", name, baud);
  return fd;
}

static void uart_tx(unsigned char byte, void *arg) {
  int len = write(*(int *) arg, &byte, 1);
  (void) len;  // Shut up GCC
}

int main(int argc, char **argv) {
  const char *baud = "115200";
  const char *port = "/dev/ttyUSB0";
  const char *iface = NULL;
  const char *bpf = NULL;
  // const char *bpf = "ether host 1:2:3:4:5:6 or ether dst ff:ff:ff:ff:ff:ff";

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      baud = argv[++i];
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      iface = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      port = argv[++i];
    } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      bpf = argv[++i];
    } else {
      return fail(
          "slipterm version %s\n"
          "Usage: %s [OPTIONS]\n"
          "  -i NETIF\t - network iface, e.g. en0, eth0. Default: NULL\n"
          "  -f FILTER\t - BPF filter. Default: %s\n"
          "  -b BAUD\t - serial speed. Default: %s\n"
          "  -p PORT\t - serial device. Default: %s\n",
          SLIPTERM_VERSION, argv[0], bpf, baud, port);
    }
  }

  // Open network interface
  char errbuf[PCAP_ERRBUF_SIZE] = "";
  if (iface == NULL) {
    pcap_if_t *devs = NULL;
    pcap_findalldevs(&devs, errbuf);
    if (devs == NULL) fail("pcap_lookupdev: %s\n", errbuf);
    iface = devs[0].name;
  }
  printf("Opened %s in live mode\n", iface);
  pcap_t *ph = pcap_open_live(iface, 0xffff, 1, 1, errbuf);
  if (ph == NULL) fail("pcap_open_live: %s\n", errbuf);
  pcap_setnonblock(ph, 1, errbuf);

  // Apply BPF to reduce noise. Let in only broadcasts and our own traffic
  if (bpf != NULL) {
    struct bpf_program bpfp;
    if (pcap_compile(ph, &bpfp, bpf, 1, 0)) fail("compile \n");
    pcap_setfilter(ph, &bpfp);
    pcap_freecode(&bpfp);
  }

  // Ok here are 3 sources we're going to listen on
  int uart_fd = open_serial(port, atoi(baud));
  int pcap_fd = pcap_get_selectable_fd(ph);
  int stdin_fd = 0;  // Keep in canonical mode to allow Ctrl-C

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialise SLIP state machine
  uint8_t slipbuf[2048];
  struct slip slip = {.buf = slipbuf, .size = sizeof(slipbuf)};

  // Main loop. Listen for input from UART, PCAP, and STDIN.
  while (s_signo == 0) {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(uart_fd, &rset);
    FD_SET(pcap_fd, &rset);
    FD_SET(stdin_fd, &rset);

    // See if there is something for us..
    int max = uart_fd > pcap_fd ? uart_fd + 1 : pcap_fd + 1;
    if (select(max, &rset, 0, 0, 0) <= 0) continue;

    // Maybe there is something on the network?
    if (FD_ISSET(pcap_fd, &rset)) {
      struct pcap_pkthdr *hdr = NULL;
      const unsigned char *pkt = NULL;
      if (pcap_next_ex(ph, &hdr, &pkt) != 1) continue;  // Yea, fetch packet
      char hd[hdr->len * 5 + 100];                      // Hexdump buffer
      printf("NET -> DEV [%d bytes]\n%s\n", hdr->len,
             hexdump(pkt, hdr->len, hd, sizeof(hd)));
      slip_send(pkt, hdr->len, uart_tx, &uart_fd);  // Forward to serial
    }

    // Maybe a device has sent us something
    if (FD_ISSET(uart_fd, &rset)) {
      uint8_t buf[BUFSIZ];
      int n = read(uart_fd, buf, sizeof(buf));   // Read
      if (n <= 0) fail("Serial line closed\n");  // If serial is closed, exit
      for (int i = 0; i < n; i++) {
        size_t len = slip_recv(buf[i], &slip);  // Pass to SLIP state machine
        if (len == 0 && slip.mode == 0) putchar(buf[i]);  // In serial mode
        if (len > 0) {                                    // We've got a frame!
          char hd[len * 5 + 100];                         // Hexdump buffer
          printf("DEV -> NET [%d bytes]\n%s\n", (int) len,
                 hexdump(slip.buf, len, hd, sizeof(hd)));
          pcap_inject(ph, slip.buf, len);  // Forward to network
        }
      }
    }

    // If a user types something and presses enter, forward to a device
    if (FD_ISSET(stdin_fd, &rset)) {
      uint8_t buf[BUFSIZ];
      int n = read(stdin_fd, buf, sizeof(buf));
      for (int i = 0; i < n; i++) uart_tx(buf[i], &uart_fd);
    }
  }

  pcap_close(ph);
  close(uart_fd);

  printf("Exiting on signal %d\n", s_signo);

  return 0;
}
