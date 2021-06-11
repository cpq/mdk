#include <sdk.h>
#include "../../tools/slip.h"

struct ppp {
  int state;  // dead, setup, auth, run, close
};

#if 0
struct lcp {
  uint8_t code;
  uint8_t id;
  uint16_t len;
} __attribute__((packed));

static inline void tx(const void *ptr, size_t len) {
  uart_tx(0x7e);
  for (size_t i = 0; i < len; i++) uart_tx(((uint8_t *) ptr)[i]);
  uart_tx(0x7e);
}
#endif

static void tx(unsigned char c, void *arg) {
  uart_tx(c);
  (void) arg;
}

static void send_frame(const void *buf, size_t len) {
  slip_send(buf, len, tx, NULL);
}

static void handle_ppp(uint8_t *buf, size_t len) {
  sdk_log("PPP: ");
  for (size_t i = 0; i < len; i++) sdk_log("%d ", buf[i]);
  sdk_log("\n");
}

static inline void blink(void) {
  gpio_write(LED1, 1);
  delay_ms(10);
  gpio_write(LED1, 0);
}

int main(void) {
  wdt_disable();  // Shut up our friend for now
  gpio_output(LED1);

  struct net_if netif = {.out = send_frame,
                         .mac = {0xd8, 0xa0, 0x1d, 1, 2, 3},
                         //.mac = {0xa4, 0x5e, 0x60, 0xb8, 0x65, 0x13},
                         .ip = 0x0700a8c0};

  struct slip slip = {.size = 1536, .buf = malloc(slip.size)};
  sdk_log("Allocated %d bytes @ %p for netif frame\n", slip.size, slip.buf);

  bool got_ipaddr = netif.ip != 0;
  unsigned long uptime_ms = 0;  // Pretend we know what time it is

  gpio_input(BTN1);
  for (;;) {
    uint8_t c;
    if (uart_rx(&c) == 0) {
      size_t len = slip_recv(c, &slip);
      if (len > 0) handle_ppp(slip.buf, len);
      if (len == 0 && slip.mode == 0) sdk_log("%c", c);
      blink();
    }
    if (gpio_read(BTN1) == false) slip_send("\x7ehi\x7e", 4, tx, NULL);
    net_poll(&netif, uptime_ms++);  // Let IP stack process things
    if (got_ipaddr == false && netif.ip != 0) {
      sdk_log("ip %x, mask %x, gw %x\n", netif.ip, netif.mask, netif.gw);
      got_ipaddr = true;
    }
  }

  return 0;  // Unreached
}
