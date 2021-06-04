#include <sdk.h>

// Per https://datatracker.ietf.org/doc/html/rfc1055
enum { END = 0300, ESC = 0333, ESC_END = 0334, ESC_ESC = 0335 };

void cn_mac_out(void *buf, size_t len) {
  uint8_t *p = buf;
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

// We could fill it up with a real device's MAC
unsigned char cn_mac_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

int main(void) {
  wdt_disable();  // Shut up our friend for now

  size_t len = 0, size = 1600;  // Serial input length, and max frame size
  uint8_t *buf = malloc(size);  // Allocate serial input buffer

  bool buffering = false;       // True when we're buffering IP packet
  unsigned long uptime_ms = 0;  // Pretend we know what time it is

  for (;;) {
    if (uart_rx(&buf[len]) == 0) {
      if (buffering) {
        if (len > 0 && buf[len] == ESC_END) {
          buf[len - 1] = END;
        } else if (len > 0 && buf[len] == ESC_ESC) {
          buf[len - 1] = ESC;
        } else if (buf[len] == END) {
          cn_mac_in(buf, len);  // Full frame received, feed to the IP stack
          len = 0;              // Flush input buffer
          buffering = false;    // Stop buffering
        } else {
          len++;
          if (len >= size) sdk_log("SLIP overflow\n"), len = 0;
        }
      } else if (buf[len] == END) {
        buffering = true;  // Start buffering
      }
    }
    cn_poll(uptime_ms++);  // Let IP stack process things
    spin(99);              // Sleep for some time.. Or maybe don't?
  }

  return 0;  // Unreached
}
