#include <sdk.h>

// On the ESP32C3 dev boards, the WS2812 LED is connected to GPIO 8
static int ws_2812_pin = 8;

// Send a set of colors to the WS2812 (Chain).  Right now, just one LED.
// Wait at least Spin(2000) time between sending ws_led commands.
static void ws_led(const uint8_t *buf, size_t len, int pin) {
  unsigned long delays[2] = {3, 9};
  for (size_t i = 0; i < len; i++) {
    // Tested: about spin(2) is the smallest you can go before
    // outrunning the chip.  11 is the threshold for
    // on-time before tripping over to the next.
    // total period must be greater than 18.
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
      int i1 = buf[i] & mask ? 0 : 1, i2 = i1 ^ 1;  // This takes some cycles
      gpio_write(pin, 1);
      spin(delays[i1]);
      gpio_write(pin, 0);
      spin(delays[i2]);
    }
  }
}

// Simple hue function for generation of smooth rainbow.
static uint8_t hueval( int value )
{
  value = value % 1536;
  if( value < 256 )
  {
    return (uint8_t)value;
  }
  else if( value < 768 )
  {
    return 255;
  }
  else if( value < 1024 )
  {
    return (uint8_t)(1023-value);
  }
  else
  {
    return 0;
  }
}

int main(void) {
  wdt_disable();
  gpio_output(ws_2812_pin);

  int frame = 0;
  for (;;) {

    // Generate rainbow pattern
    uint8_t leddata[3];
    leddata[0] = hueval(frame);
    leddata[1] = hueval(frame+512);
    leddata[2] = hueval(frame+1024);
    ws_led( leddata, 3, ws_2812_pin );

    frame++;
    delay_ms(1);
  }

  return 0;
}

