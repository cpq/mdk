// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once
#include "mdk.h"

struct spi {
  int miso, mosi, clk, cs, spin;
};

void spi_begin(struct spi *spi);
void spi_end(struct spi *spi);
bool spi_init(struct spi *spi);
unsigned char spi_txn(struct spi *spi, unsigned char tx);
