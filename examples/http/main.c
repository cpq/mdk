#include <sdk.h>

static unsigned readn(struct spi *spi, uint8_t reg, int n) {
  unsigned rx = 0;
  spi_begin(spi, 0);
  spi_txn(spi, reg | 0x80);
  for (int i = 0; i < n; i++) rx <<= 8, rx |= spi_txn(spi, 0);
  spi_end(spi, 0);
  return rx;
}

static void write8(struct spi *spi, uint8_t reg, uint8_t val) {
  spi_begin(spi, 0);
  spi_txn(spi, (uint8_t)(reg & ~0x80));
  spi_txn(spi, val);
  spi_end(spi, 0);
}

uint16_t swap16(uint16_t val) {
  uint8_t data[2] = {0, 0};
  memcpy(&data, &val, sizeof(data));
  return (uint16_t)((uint16_t) data[1] | (((uint16_t) data[0]) << 8));
}

// Taken from the BME280 datasheet, 4.2.3
int32_t read_temp(struct spi *spi) {
  int32_t t = (int32_t)(readn(spi, 0xfa, 3) >> 4);  // Read temperature reg

  uint16_t c1 = (uint16_t) readn(spi, 0x88, 2);  // dig_T1
  uint16_t c2 = (uint16_t) readn(spi, 0x8a, 2);  // dig_T2
  uint16_t c3 = (uint16_t) readn(spi, 0x8c, 2);  // dig_T3

  int32_t t1 = (int32_t) swap16(c1);
  int32_t t2 = (int32_t)(int16_t) swap16(c2);
  int32_t t3 = (int32_t)(int16_t) swap16(c3);

  int32_t var1 = ((((t >> 3) - (t1 << 1))) * t2) >> 11;
  int32_t var2 = (((((t >> 4) - t1) * ((t >> 4) - t1)) >> 12) * t3) >> 14;

  return ((var1 + var2) * 5 + 128) >> 8;
}

int main(void) {
  wdt_disable();

  struct spi bme280 = {.mosi = 23, .miso = 19, .clk = 18, .cs = {5, -1, -1}};
  spi_init(&bme280);

  write8(&bme280, 0xe0, 0xb6);          // Soft reset
  spin(999999);                         // Wait until reset
  write8(&bme280, 0xf4, 0);             // REG_CONTROL, MODE_SLEEP
  write8(&bme280, 0xf5, (3 << 5));      // REG_CONFIG, filter = off, 20ms
  write8(&bme280, 0xf4, (1 << 5) | 3);  // REG_CONFIG, MODE_NORMAL

  for (;;) {
    sdk_log("Chip ID: %d, expecting 96\n", readn(&bme280, 0xd0, 1));
    int temp = read_temp(&bme280);
    sdk_log("Temp: %d.%d\n", temp / 100, temp % 100);
    spin(9999999);
  }

  return 0;
}
