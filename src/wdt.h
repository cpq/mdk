// Copyright (c) 2021 Cesanta
// All rights reserved

#if defined(ESP32C3)
static inline void wdt_disable(void) {
  REG(C3_RTCCNTL)[42] = 0x50d83aa1;  // Disable write protection
  REG(C3_RTCCNTL)[36] = 0;           // Disable RTC WDT

  // bootloader_super_wdt_auto_feed()
  REG(C3_RTCCNTL)[44] = 0x8F1D312A;
  REG(C3_RTCCNTL)[43] |= BIT(31);
  REG(C3_RTCCNTL)[45] = 0;

  REG(C3_TIMERGROUP0)[18] = 0;  // Disable T0 WDT
  REG(C3_TIMERGROUP1)[18] = 9;  // Disable T1 WDT
}
#elif defined(ESP32)
static inline void wdt_feed(void) {
  REG(ESP32_RTCCNTL)[40] |= BIT(31);
}

static inline void wdt_disable(void) {
  REG(ESP32_RTCCNTL)[41] = 0x50d83aa1;  // Disable write protection
  wdt_feed();
  REG(ESP32_RTCCNTL)[35] = 0;      // Disable RTC WDT
  REG(ESP32_TIMERGROUP0)[18] = 0;  // Disable task WDT
  REG(ESP32_TIMERGROUP1)[18] = 0;  // Disable task WDT
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
static inline void wdt_disable(void) {
}
#else
#error "Ouch"
#endif
