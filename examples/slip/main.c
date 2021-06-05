#include <sdk.h>

// Per https://datatracker.ietf.org/doc/html/rfc1055
enum { END = 0300, ESC = 0333, ESC_END = 0334, ESC_ESC = 0335 };

static void send_frame(const void *buf, size_t len) {
  const uint8_t *p = buf;
  uart_tx(END);
  for (size_t i = 0; i < len; i++) {
    if (p[i] == END) {
      uart_tx(ESC);
      uart_tx(ESC_END);
    } else if (p[i] == ESC) {
      uart_tx(ESC);
      uart_tx(ESC_ESC);
    } else {
      uart_tx(p[i]);
    }
  }
  uart_tx(END);
}

int main(void) {
  wdt_disable();  // Shut up our friend for now

  size_t len = 0, size = 1600;  // Serial input length, and max frame size
  uint8_t *buf = malloc(size);  // Allocate serial input buffer

  struct cn_if netif = {.name = "slip",
                        .out = send_frame,
                        .mac = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};

  bool got_ipaddr = false;
  bool buffering = false;       // True when we're buffering IP packet
  unsigned long uptime_ms = 0;  // Pretend we know what time it is

  sdk_log("Starting SLIP, max frame len %u\n", size);
  for (;;) {
    if (uart_rx(&buf[len]) == 0) {
      if (buffering) {
        if (len > 0 && buf[len] == ESC_END) {
          buf[len - 1] = END;
        } else if (len > 0 && buf[len] == ESC_ESC) {
          buf[len - 1] = ESC;
        } else if (buf[len] == END) {
          cn_input(&netif, buf, len);  // Full frame received
          len = 0;                     // Flush input buffer
          buffering = false;           // Stop buffering
        } else {
          len++;
          if (len >= size) sdk_log("SLIP overflow\n"), len = 0;
        }
      } else if (buf[len] == END) {
        buffering = true;  // Start buffering
      }
    }
    cn_poll(&netif, uptime_ms++);  // Let IP stack process things
    if (got_ipaddr == false && netif.ip != 0) {
      sdk_log("ip %x, mask %x, gw %x\n", netif.ip, netif.mask, netif.gw);
      got_ipaddr = true;
    }
  }

  return 0;  // Unreached
}
