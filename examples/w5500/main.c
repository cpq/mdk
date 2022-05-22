// Copyright (c) 2021 Cesanta
// All rights reserved
//
// Wiznet W5500 Ethernet example
// This example required mip network stack, https://github.com/cesanta/mip

#include <sdk.h>

#include "drivers/w5500.h"
#include "mip.h"
#include "mongoose.h"

extern void mg_attach_mip(struct mg_mgr *mgr, struct mip_if *ifp);

int64_t mg_millis(void) {
  return (int64_t) time_us() / 1000;
}

static void tx(struct mip_if *ifp) {
  struct w5500 *wiz = ifp->txdata;
  w5500_tx(wiz, ifp->frame, (uint16_t) ifp->frame_len);
  printf("TX %u\n", ifp->frame_len);
}

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_POLL) return;
  printf("%lu %d %p %p", c->id, ev, ev_data, fn_data);
  if (ev == MG_EV_OPEN) {
    c->is_hexdumping = 1;
  }
}

static void timer_fn(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  mg_http_connect(mgr, "http://cesanta.com", fn, NULL);
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

  // struct slip slip = {.size = 1536, .buf = malloc(slip.size)};
  // printf("Allocated %d bytes @ %p for netif frame\n", slip.size, slip.buf);

  uint8_t frame[2048];
  struct mip_if mif = {.tx = tx,
                       .txdata = &wiz,
                       .frame = frame,
                       .mac = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
                       .frame_max_size = sizeof(frame)};

  struct mg_mgr mgr;
  struct mg_timer t;
  mg_mgr_init(&mgr);
  mg_attach_mip(&mgr, &mif);
  mg_timer_init(&t, 300, MG_TIMER_REPEAT, timer_fn, &mgr);
  mg_listen(&mgr, "udp://0.0.0.0:1234", fn, NULL);

  uint8_t s1 = 0, s2 = 0;
  for (;;) {
    uint8_t buf[1600];
    size_t len = w5500_rx(&wiz, buf, (uint16_t) sizeof(buf));
    if (len > 0) {
      if (len > mif.frame_max_size) continue;
      mif.frame_len = len;
      memcpy(mif.frame, buf, len);  // Feed MIP
      mip_rx(&mif);
      printf("RX: %u\n", len);
    }
    mip_poll(&mif, (uint64_t) mg_millis());
    s2 = w5500_status(&wiz);
    if (s1 != s2) s1 = s2, printf("Ethernet status changed to: %x\n", s1);
  }

  return 0;
}
