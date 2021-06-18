#include <sdk.h>
#include "../../tools/slip.h"

static void tx(unsigned char c, void *arg) {
  uart_write(0, c);
  (void) arg;
}

static void send_frame(const void *buf, size_t len) {
  slip_send(buf, len, tx, NULL);
}

int main(void) {
  wdt_disable();  // Shut up our friend for now

  struct net_if netif = {.out = send_frame,
                         .mac = {0xd8, 0xa0, 0x1d, 1, 2, 3},
                         //.mac = {0xa4, 0x5e, 0x60, 0xb8, 0x65, 0x13},
                         .ip = 0x0700a8c0};

  struct slip slip = {.size = 1536, .buf = malloc(slip.size)};
  sdk_log("Allocated %d bytes @ %p for netif frame\n", slip.size, slip.buf);

  bool got_ipaddr = netif.ip != 0;
  unsigned long uptime_ms = 0;  // Pretend we know what time it is
  for (;;) {
    uint8_t c;
    if (uart_read(0, &c)) {
      size_t len = slip_recv(c, &slip);
      if (len > 0) net_input(&netif, slip.buf, slip.size, len);
      if (len == 0 && slip.mode == 0) sdk_log("%c", c);
    }
    net_poll(&netif, uptime_ms++);  // Let IP stack process things
    if (got_ipaddr == false && netif.ip != 0) {
      sdk_log("ip %x, mask %x, gw %x\n", netif.ip, netif.mask, netif.gw);
      got_ipaddr = true;
    }
  }

  return 0;  // Unreached
}
