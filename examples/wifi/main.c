#include <sdk.h>

static void cb(void) {
  sdk_log("calleth %s\n", cb);
}

void getmac(uint8_t *mac) {
  for (uint8_t i = 0; i < 6; i++) mac[i] = i;
}

int main(void) {
  wdt_disable();

  delay_ms(1000);
#if 0
  DPORT_SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG,
                          SYSTEM_WIFI_CLK_WIFI_BT_COMMON_M);
  DPORT_CLEAR_PERI_REG_MASK(SYSTEM_CORE_RST_EN_REG, 0);
#endif
  // REG(C3_SYSCON)[5] |= 0x78078f;  // Enable wifi clock
  REG(C3_SYSCON)[5] |= 0xfb9fcf;  // Enable wifi clock
  REG(C3_SYSCON)[6] &= ~0U;       // Reset
  delay_ms(100);

  uint8_t x[6];
  getmac(x);
  // extern int wifi_get_macaddr(uint8_t *);
  // sdk_log("-> %d\n", wifi_get_macaddr(x));
  delay_ms(1000);

  // extern void ic_register_rx_cb(void (*)(void));
  // ic_register_rx_cb(cb);
  (void) cb;

  // sdk_log("calling init... ");
  // extern int esp_wifi_init_internal(struct wifi_init_config *);
  // int res = esp_wifi_init_internal(&c);
  // sdk_log("result: %d\n", res);

  // wifi_rf_phy_enable = 0x4000187c;
  //((void (*)(void))(uintptr_t) 0x4000187c)();
  // int v = 0;
  for (;;) {
    // uint8_t mac[6] = {};
    // wifi_mac(mac);
    // v = ((int (*)(void)) 0x400015e0)();  // ic_mac_init
    // v = ((int (*)(void)) 0x400018a8)();  // wifi_is_started
    // int v = wifi_get_macaddr(mac);
    // sdk_log("WiFi mac: %x:%x:%x:%x:%x:%x %d\n", mac[0], mac[1], mac[2],
    // mac[3], mac[4], mac[5], v);
    delay_ms(1500);
  }

  return 0;
}
