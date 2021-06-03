# BME280 sensor example

Pinout is according to the C source: MOSI - 23, MISO - 19, CLK - 18, CS = 5:

```c
  struct spi bme280 = {.mosi = 23, .miso = 19, .clk = 18, .cs = {5, -1, -1}};
```

Build, flash and monitor:

```
Chip ID: 96, expecting 96
Temp: 23.69
Chip ID: 96, expecting 96
Temp: 25.5
```
