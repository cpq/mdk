// Copyright (c) 2021 Cesanta
// All rights reserved

#include "sdk.h"

// TODO(cpq): make this configurable with accurate frequency
static inline void spi_clock_delay(void) {
  spin(9);
}

bool spi_init(struct spi *spi) {
  if (spi->miso < 0 || spi->mosi < 0 || spi->clk < 0) return false;
  gpio_input(spi->miso);
  gpio_output(spi->mosi);
  gpio_output(spi->clk);
  for (size_t i = 0; i < sizeof(spi->cs) / sizeof(spi->cs[0]); i++) {
    if (spi->cs[i] < 0) continue;
    gpio_output(spi->cs[i]);
    gpio_write(spi->cs[i], 1);
  }
  return true;
}

// Send a byte, and return a received byte
unsigned char spi_txn(struct spi *spi, unsigned char tx) {
  unsigned char rx = 0;
  for (int i = 0; i < 8; i++) {
    gpio_write(spi->mosi, tx & 0x80);   // Set mosi
    spi_clock_delay();                  // Wait half cycle
    gpio_write(spi->clk, 1);            // Clock high
    rx = (unsigned char) (rx << 1);     // "rx <<= 1" gives warning??
    if (gpio_read(spi->miso)) rx |= 1;  // Read mosi
    spi_clock_delay();                  // Wait half cycle
    gpio_write(spi->clk, 0);            // Clock low
    tx = (unsigned char) (tx << 1);     // Again, avoid warning
  }
  return rx;  // Return the received byte
}
