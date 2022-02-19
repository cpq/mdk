// Copyright (c) 2021 Cesanta
// All rights reserved
//
// Wiznet W5500 Ethernet example
// This example required mip network stack, https://github.com/cesanta/mip

#include <sdk.h>

#include "drivers/w5500.h"
#include "slip.h"

#if 0
static void ether_tx(struct mip_if *ifp) {
  int n = w5500_tx(ifp->userdata, ifp->frame, (uint16_t) ifp->frame_len);
  // printf("tx: %d %x\n", n, w5500_r1(ifp->userdata, W5500_CR, 0x2e));
}
#endif

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
  for (;;) {
    uint8_t c;
    if (uart_read(0, &c)) {
      size_t len = slip_recv(c, &slip);
      if (len > 0) {
        int n = w5500_tx(&wiz, slip.buf, (uint16_t) len);
        printf("TX: %d/%d %x\n", n, (int) len, w5500_r1(&wiz, W5500_CR, 0x2e));
      }
      if (len == 0 && slip.mode == 0) putchar(c);
    }
    uint8_t buf[1600];
    size_t len = w5500_rx(&wiz, buf, (uint16_t) sizeof(buf));
    if (len > 0) {
      slip_send(buf, len, uart_tx, NULL);
      printf("RX: %d\n", (int) len);
    }
  }

#if 0
  uint8_t frame[2048];
  struct mip_if mif = {.tx = tx,
                       .userdata = &wiz,
                       .frame = frame,
                       .frame_max_size = sizeof(frame)};

  int pin = 9;
  gpio_input(pin);
  for (;;) {
    mif.frame_len = w5500_rx(&wiz, mif.frame, (uint16_t) mif.frame_max_size);
    if (mif.frame_len > 0) {
      printf("rx: %u\n", mif.frame_len);
      // if (mif.frame_len < 100) hexdump(mif.frame, mif.frame_len);
      mip_rx(&mif);
    }
    mip_poll(&mif, time_us() / 1000);
    delay_ms(1);
    // printf("%08llx %lx %d %x\n", time_us(), csr_read_mvendorid(),
    //       gpio_read(pin), w5500_r1(&wiz, W5500_CR, 0x2e));
  }
#endif

  return 0;
}
