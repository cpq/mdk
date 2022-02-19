// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once
#include "gpio.h"

struct spi {
  int miso, mosi, clk, cs, spin;
};

static inline void spi_begin(struct spi *spi) {
  gpio_write(spi->cs, 0);
}

static inline void spi_end(struct spi *spi) {
  gpio_write(spi->cs, 1);
}

static inline bool spi_init(struct spi *spi) {
  if (spi->miso < 0 || spi->mosi < 0 || spi->clk < 0) return false;
  gpio_input(spi->miso);
  gpio_output(spi->mosi);
  gpio_output(spi->clk);
  if (spi->cs >= 0) {
    gpio_output(spi->cs);
    gpio_write(spi->cs, 1);
  }
  return true;
}

// Send a byte, and return a received byte
static inline unsigned char spi_txn(struct spi *spi, unsigned char tx) {
  unsigned count = spi->spin <= 0 ? 9 : (unsigned) spi->spin;
  unsigned char rx = 0;
  for (int i = 0; i < 8; i++) {
    gpio_write(spi->mosi, tx & 0x80);   // Set mosi
    spin(count);                        // Wait half cycle
    gpio_write(spi->clk, 1);            // Clock high
    rx = (unsigned char) (rx << 1);     // "rx <<= 1" gives warning??
    if (gpio_read(spi->miso)) rx |= 1;  // Read miso
    spin(count);                        // Wait half cycle
    gpio_write(spi->clk, 0);            // Clock low
    tx = (unsigned char) (tx << 1);     // Again, avoid warning
  }
  return rx;  // Return the received byte
}
