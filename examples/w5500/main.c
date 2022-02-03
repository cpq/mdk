#include <sdk.h>
#include "drivers/w5500.h"
#include "mip.h"

static int led_pin = LED1;  // To override: make EXTRA_CFLAGS=-DLED1=5

static void tx(struct mip_if *ifp) {
  w5500_tx(ifp->userdata, ifp->frame, (uint16_t) ifp->frame_len);
}

int main(void) {
  wdt_disable();
  gpio_output(led_pin);

  struct spi spi = {.mosi = 23, .miso = 19, .clk = 18, .cs = {5, -1, -1}};
  spi_init(&spi);

  struct w5500 wiz = {.spi = &spi, .cs = 5, .mac = {0xaa, 0xbb, 0xcc, 1, 2, 3}};
  w5500_init(&wiz);

  uint8_t frame[2048];
  struct mip_if mif = {.tx = tx,
                       .userdata = &wiz,
                       .frame = frame,
                       .frame_max_size = sizeof(frame)};

  for (;;) {
    mif.frame_len = w5500_rx(&wiz, mif.frame, (uint16_t) mif.frame_max_size);
    mip_rx(&mif);
    mip_poll(&mif, time_us() / 1000);
  }

  return 0;
}
