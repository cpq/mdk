// Copyright (c) 2021 Cesanta
// All rights reserved

#if defined(ESP32C3)
#define RTC_CNTL_WDTCONFIG0_REG REG(0x60008090)
#define RTC_CNTL_WDTWPROTECT_REG REG(0x600080a8)
static inline void wdt_disable(void) {
  RTC_CNTL_WDTWPROTECT_REG[0] = 0x50d83aa1;  // Disable write protection
  RTC_CNTL_WDTCONFIG0_REG[0] &= BIT(31);     // Disable RTC WDT
}
#elif defined(ESP32)
#define RTC_CNTL_WDTCONFIG0_REG REG(0X3ff4808c)
#define TIMG0_T0_WDTCONFIG0_REG REG(0X3ff5f048)
#define RTC_CNTL_WDTFEED_REG REG(0X3ff480a0)
#define RTC_CNTL_WDTWPROTECT_REG REG(0x3FF480A4)

static inline void wdt_feed(void) {
  RTC_CNTL_WDTFEED_REG[0] |= BIT(31);
}

static inline void wdt_disable(void) {
  RTC_CNTL_WDTWPROTECT_REG[0] = 0x50d83aa1;  // Disable write protection
  RTC_CNTL_WDTCONFIG0_REG[0] &= BIT(31);     // Disable RTC WDT
  TIMG0_T0_WDTCONFIG0_REG[0] &= BIT(31);     // Disable task WDT
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
static inline void wdt_disable(void) {
}
#else
#error "Ouch"
#endif
