struct spi {
  int miso, mosi, clk, freq;
};

int spi_init(struct spi *spi);
int spi_txn(struct spi *spi);
