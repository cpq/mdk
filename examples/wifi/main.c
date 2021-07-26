#include <sdk.h>

static void cb(void) {
  uint8_t mac[6] = {};
  wifi_get_mac_addr(mac);

  unsigned cpu_freq = ((unsigned (*)(void)) 0x40000584)();  // esp32c3.rom.ld
  sdk_log("WiFi mac: %x:%x:%x:%x:%x:%x, cpu_freq: %d\n", mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5], cpu_freq);
}

int main(void) {
  wdt_disable();

  delay_ms(1000);
  // REG(C3_SYSCON)[5] |= 0x78078f;  // Enable wifi clock
  // REG(C3_SYSCON)[5] |= 0xfb9fcf;  // Enable wifi clock
  // REG(C3_SYSCON)[6] &= ~0U;  // Reset

  //((void (*)(void *, void *, void *)) 0x400414d2)(cb, 0, cb);  // ppRxPkt()
  //((void (*)(void *)) 0x4003dc40)(cb);  // lmacRxDone()
  //((void (*)(void *)) 0x40001720)(cb);  // ppTask()

  for (;;) {
    cb();
    delay_ms(1500);
    //((void (*)(void *)) 0x40001720)(cb);  // ppTask()
  }

  return 0;
}
