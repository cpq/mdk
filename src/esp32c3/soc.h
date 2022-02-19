// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#define C3_SYSTEM 0x600c0000
#define C3_SENSITIVE 0x600c1000
#define C3_INTERRUPT 0x600c2000
#define C3_EXTMEM 0x600c4000
#define C3_MMU_TABLE 0x600c5000
#define C3_AES 0x6003a000
#define C3_SHA 0x6003b000
#define C3_RSA 0x6003c000
#define C3_HMAC 0x6003e000
#define C3_DIGITAL_SIGNATURE 0x6003d000
#define C3_GDMA 0x6003f000
#define C3_ASSIST_DEBUG 0x600ce000
#define C3_DEDICATED_GPIO 0x600cf000
#define C3_WORLD_CNTL 0x600d0000
#define C3_DPORT_END 0x600d3FFC
#define C3_UART 0x60000000
#define C3_SPI1 0x60002000
#define C3_SPI0 0x60003000
#define C3_GPIO 0x60004000
#define C3_FE2 0x60005000
#define C3_FE 0x60006000
#define C3_RTCCNTL 0x60008000
#define C3_IO_MUX 0x60009000
#define C3_RTC_I2C 0x6000e000
#define C3_UART1 0x60010000
#define C3_I2C_EXT 0x60013000
#define C3_UHCI0 0x60014000
#define C3_RMT 0x60016000
#define C3_LEDC 0x60019000
#define C3_EFUSE 0x60008800
#define C3_NRX 0x6001CC00
#define C3_BB 0x6001D000
#define C3_TIMERGROUP0 0x6001F000
#define C3_TIMERGROUP1 0x60020000
#define C3_SYSTIMER 0x60023000
#define C3_SPI2 0x60024000
#define C3_SYSCON 0x60026000
#define C3_APB_CTRL 0x60026000
#define C3_TWAI 0x6002B000
#define C3_I2S0 0x6002D000
#define C3_APB_SARADC 0x60040000
#define C3_AES_XTS 0x600CC000

static inline uint64_t systick(void) {
  REG(C3_SYSTIMER)[1] = BIT(30);  // TRM 10.5
  spin(1);
  return ((uint64_t) REG(C3_SYSTIMER)[16] << 32) | REG(C3_SYSTIMER)[17];
}

static inline uint64_t time_us(void) {
  return systick() >> 4;
#if 0
  REG(C3_TIMERGROUP0)[3] = BIT(31);  // TRM 8.3.1, request to fetch timer value
  spin(1);                           // Is there a better way to wait?
  uint32_t hi = REG(C3_TIMERGROUP0)[2];  // High 20 bits
  uint32_t lo = REG(C3_TIMERGROUP0)[1];  // Low 32 bits
  return ((uint64_t) hi << 32) | lo;
#endif
}

static inline void delay_us(unsigned long us) {
  //((void (*)(unsigned long)) 0x40000050)(us);  // ets_delay_us(us);
  uint64_t until = time_us() + us;  // Handler counter wrap
  while (time_us() < until) spin(1);
}

static inline void delay_ms(unsigned long ms) {
  delay_us(ms * 1000);
}

static inline void ledc_enable(void) {
  REG(C3_SYSTEM)[4] |= BIT(11);                // Enable LEDC clock
  REG(C3_LEDC)[52] = 1U | BIT(31);             // Use APB clock, 15.3.2.1
  REG(C3_LEDC)[40] = 15U | BIT(12) | BIT(25);  // Timer 0 overflow 14-bit
  REG(C3_LEDC)[0] = BIT(2) | BIT(4);           // Chan 0 config
  REG(C3_LEDC)[1] = 4096;                      // Chan 0 Hpoint
  REG(C3_LEDC)[2] = 1 << 4;                    // Chan 0 Lpoint
}

static inline void wdt_disable(void) {
  REG(C3_RTCCNTL)[42] = 0x50d83aa1;  // Disable write protection
  // REG(C3_RTCCNTL)[36] &= BIT(31);    // Disable RTC WDT
  REG(C3_RTCCNTL)[36] = 0;  // Disable RTC WDT
  REG(C3_RTCCNTL)[35] = 0;  // Disable

  // bootloader_super_wdt_auto_feed()
  REG(C3_RTCCNTL)[44] = 0x8F1D312A;
  REG(C3_RTCCNTL)[43] |= BIT(31);
  REG(C3_RTCCNTL)[45] = 0;

  REG(C3_TIMERGROUP0)[63] &= ~BIT(9);  // TIMG_REGCLK -> disable TIMG_WDT_CLK
  REG(C3_TIMERGROUP0)[18] = 0;         // Disable TG0 WDT
  REG(C3_TIMERGROUP1)[18] = 0;         // Disable TG1 WDT
}

static inline void wifi_get_mac_addr(uint8_t mac[6]) {
  uint32_t a = REG(C3_EFUSE)[17], b = REG(C3_EFUSE)[18];
  mac[0] = (b >> 8) & 255, mac[1] = b & 255, mac[2] = (uint8_t) (a >> 24) & 255;
  mac[3] = (a >> 16) & 255, mac[4] = (a >> 8) & 255, mac[5] = a & 255;
}

static inline void soc_init(void) {
  // Init clock. TRM 6.2.4.1
  REG(C3_SYSTEM)[2] &= ~3U;
  REG(C3_SYSTEM)[2] |= BIT(0) | BIT(2);
  REG(C3_SYSTEM)[22] = BIT(19) | (40U << 12) | BIT(10);
  // REG(C3_RTCCNTL)[47] = 0; // RTC_APB_FREQ_REG -> freq >> 12
  ((void (*)(int)) 0x40000588)(160);  // ets_update_cpu_frequency(160)

#if 0
  // Configure system clock timer, TRM 8.3.1, 8.9
  REG(C3_TIMERGROUP0)[1] = REG(C3_TIMERGROUP0)[2] = 0UL;  // Reset LO and HI
  REG(C3_TIMERGROUP0)[8] = 0;                             // Trigger reload
  REG(C3_TIMERGROUP0)[0] = (83U << 13) | BIT(12) | BIT(29) | BIT(30) | BIT(31);
#endif
}

#define CSR_READ(name)                             \
  static inline uint32_t csr_read_##name(void) {   \
    uint32_t v;                                    \
    asm volatile("csrr %0, " #name : "=r"(v) : :); \
    return v;                                      \
  }
#define CSR_WRITE(name)                                              \
  static inline uint32_t csr_write_##name(uint32_t v) {              \
    uint32_t prev;                                                   \
    asm volatile("csrrw %0, " #name ", %1" : "=r"(prev) : "r"(v) :); \
    return prev;                                                     \
  }

CSR_READ(mvendorid);
CSR_READ(mstatus);
CSR_WRITE(mstatus);
