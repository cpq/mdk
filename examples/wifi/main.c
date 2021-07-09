#include <sdk.h>

static void cb(void) {
  uint8_t mac[6] = {};
  wifi_get_mac_addr(mac);
  sdk_log("WiFi mac: %x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
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
  }

  return 0;
}
