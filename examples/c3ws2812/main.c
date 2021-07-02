// Copyright (c) Charles Lohr
// All rights reserved

#include <sdk.h>

// On the ESP32C3 dev boards, the WS2812 LED is connected to GPIO 8
static int ws_2812_pin = 8;

// Simple hue function for generation of smooth rainbow.
static uint8_t hueval(int value) {
  value = value % 1536;
  if (value < 256) {
    return (uint8_t) value;
  } else if (value < 768) {
    return 255;
  } else if (value < 1024) {
    return (uint8_t)(1023 - value);
  } else {
    return 0;
  }
}

static void blink(int count, unsigned long delay_millis) {
  uint8_t green[3] = {100, 0, 0}, black[3] = {0, 0, 0};
  for (int i = 0; i < count; i++) {
    ws2812_show(ws_2812_pin, green, sizeof(green));
    delay_ms(delay_millis);
    ws2812_show(ws_2812_pin, black, sizeof(black));
    delay_ms(delay_millis);
  }
}

static void rainbow(int count) {
  for (int i = 0; i < count; i++) {
    uint8_t rgb[3] = {hueval(i), hueval(i + 512), hueval(i + 1024)};
    ws2812_show(ws_2812_pin, rgb, sizeof(rgb));
    delay_ms(1);
  }
}

int main(void) {
  wdt_disable();
  gpio_output(ws_2812_pin);

  for (;;) {
    blink(3, 300);
    rainbow(2500);
  }

  return 0;
}
