// Copyright (c) 2021 Cesanta
// All rights reserved
//
// Wiznet W5500 Ethernet example
// This example required mip network stack, https://github.com/cesanta/mip

#include <sdk.h>

#include "drivers/w5500.h"
#include "slip.h"

static void uart_tx(unsigned char c, void *arg) {
  uart_write(0, c);
  (void) arg;
}

int main(void) {
  wdt_disable();
  delay_ms(200);

  struct spi spi = {.mosi = 0, .miso = 2, .clk = 1, .spin = 4, .cs = 10};
  bool spi_ok = spi_init(&spi);
  printf("SPI init: %d\n", spi_ok);

  struct w5500 wiz = {.spi = &spi,
                      .txn = (uint8_t(*)(void *, uint8_t)) spi_txn,
                      .begin = (void (*)(void *)) spi_begin,
                      .end = (void (*)(void *)) spi_end,
                      .mac = {0xaa, 0xbb, 0xcc, 1, 2, 3}};
  bool wiz_ok = w5500_init(&wiz);
  printf("Wiz init: %d\n", wiz_ok);

  struct slip slip = {.size = 1536, .buf = malloc(slip.size)};
  printf("Allocated %d bytes @ %p for netif frame\n", slip.size, slip.buf);

  uint8_t c, s1 = 0, s2 = 0;
  for (;;) {
    while (uart_read(0, &c)) {
      size_t len = slip_recv(c, &slip);
      if (len > 0) {
        printf("TX %u\n", len);
        // hexdump(slip.buf, len);
        w5500_tx(&wiz, slip.buf, (uint16_t) len);
      }
      if (len == 0 && slip.mode == 0) putchar(c);
    }
    uint8_t buf[1600];
    size_t len = w5500_rx(&wiz, buf, (uint16_t) sizeof(buf));
    if (len > 0) {
      slip_send(buf, len, uart_tx, NULL);
      printf("RX: %u\n", len);
    }
    s2 = w5500_status(&wiz);
    if (s1 != s2) s1 = s2, printf("Ethernet status changed to: %x\n", s1);
  }

  return 0;
}
