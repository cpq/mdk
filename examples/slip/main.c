#include <sdk.h>
#include "../../tools/slip.h"

static void tx(unsigned char c, void *arg) {
  uart_tx(c);
  (void) arg;
}

static void send_frame(const void *buf, size_t len) {
  slip_send(buf, len, tx, NULL);
}

int main(void) {
  wdt_disable();  // Shut up our friend for now

  struct cnip_if netif = {.out = send_frame,
                          .mtu = 1600,
                          .mac = {0xd8, 0xa0, 0x1d, 1, 2, 3},
                          .ip = 0x0700a8c0};

  struct slip slip = {.size = netif.mtu, .buf = malloc(netif.mtu)};
  sdk_log("Allocated %d bytes @ %p for netif frame\n", netif.mtu, slip.buf);

  bool got_ipaddr = netif.ip != 0;
  unsigned long uptime_ms = 0;  // Pretend we know what time it is
  for (;;) {
    uint8_t c;
    if (uart_rx(&c) == 0) {
      size_t len = slip_recv(c, &slip);
      if (len > 0) cnip_input(&netif, slip.buf, len);
      if (len == 0 && slip.mode == 0) sdk_log("%c", c);
    }
    cnip_poll(&netif, uptime_ms++);  // Let IP stack process things
    if (got_ipaddr == false && netif.ip != 0) {
      sdk_log("ip %x, mask %x, gw %x\n", netif.ip, netif.mask, netif.gw);
      got_ipaddr = true;
    }
  }

  return 0;  // Unreached
}
