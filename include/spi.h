#pragma once
#include "gpio.h"

struct spi {
  int miso, mosi, clk, cs[3];
};

bool spi_init(struct spi *spi);
unsigned char spi_txn(struct spi *spi, unsigned char tx);

static inline void spi_begin(struct spi *spi, int cs) {
  gpio_write(spi->cs[cs], 0);
}

static inline void spi_end(struct spi *spi, int cs) {
  gpio_write(spi->cs[cs], 1);
}

