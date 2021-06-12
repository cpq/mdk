#include <sdk.h>

// On the ESP32C3 dev boards, the WS2812 LED is connected to GPIO 8
static int ws_2812_pin = 8;

// Send a set of colors to the WS2812 (Chain).  Right now, just one LED.
// Wait at least Spin(2000) time between sending ws_led commands. 
static void ws_led( const uint8_t * s, int bytes, int ws2812_pin )
{
  int i;
  for( i = 0; i < bytes; i++ )
  {
    int v = s[i];
    int b = 0x80;
    do
    {
      //Tested: about spin(2) is the smallest you can go before
      // outrunning the chip.  11 is the threshold for
      //on-time before tripping over to the next.
      //total period must be greater than 18.
      if( v & b )
      {
        gpio_write( ws2812_pin, 1 );
        spin(18);
        gpio_write( ws2812_pin, 0 );
        spin(6);
      }
      else
      {
        gpio_write( ws2812_pin, 1 );
        spin(6);
        gpio_write( ws2812_pin, 0 );
        spin(18);
      }
      b >>= 1;
    } while( b );
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
    spin(300000); //Wait approx 1/100th of a second.
  }

  return 0;
}

