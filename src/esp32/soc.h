// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#define ESP32_DPORT 0x3ff00000
#define ESP32_AES 0x3ff01000
#define ESP32_RSA 0x3ff02000
#define ESP32_SHA 0x3ff03000
#define ESP32_FLASH_MMU_TABLE_PRO 0x3ff10000
#define ESP32_FLASH_MMU_TABLE_APP 0x3ff12000
#define ESP32_DPORT_END 0x3ff13FFC
#define ESP32_UART 0x3ff40000
#define ESP32_SPI1 0x3ff42000
#define ESP32_SPI0 0x3ff43000
#define ESP32_GPIO 0x3ff44000
#define ESP32_GPIO_SD 0x3ff44f00
#define ESP32_FE2 0x3ff45000
#define ESP32_FE 0x3ff46000
#define ESP32_FRC_TIMER 0x3ff47000
#define ESP32_RTCCNTL 0x3ff48000
#define ESP32_RTCIO 0x3ff48400
#define ESP32_SENS 0x3ff48800
#define ESP32_RTC_I2C 0x3ff48C00
#define ESP32_IO_MUX 0x3ff49000
#define ESP32_HINF 0x3ff4B000
#define ESP32_UHCI1 0x3ff4C000
#define ESP32_I2S 0x3ff4F000
#define ESP32_UART1 0x3ff50000
#define ESP32_BT 0x3ff51000
#define ESP32_I2C_EXT 0x3ff53000
#define ESP32_UHCI0 0x3ff54000
#define ESP32_SLCHOST 0x3ff55000
#define ESP32_RMT 0x3ff56000
#define ESP32_PCNT 0x3ff57000
#define ESP32_SLC 0x3ff58000
#define ESP32_LEDC 0x3ff59000
#define ESP32_EFUSE 0x3ff5A000
#define ESP32_SPI_ENCRYPT 0x3ff5B000
#define ESP32_NRX 0x3ff5CC00
#define ESP32_BB 0x3ff5D000
#define ESP32_PWM 0x3ff5E000
#define ESP32_TIMERGROUP0 0x3ff5F000
#define ESP32_TIMERGROUP1 0x3ff60000
#define ESP32_RTCMEM0 0x3ff61000
#define ESP32_RTCMEM1 0x3ff62000
#define ESP32_RTCMEM2 0x3ff63000
#define ESP32_SPI2 0x3ff64000
#define ESP32_SPI3 0x3ff65000
#define ESP32_SYSCON 0x3ff66000
#define ESP32_APB_CTRL 0x3ff66000  // Old name for SYSCON, to be removed
#define ESP32_I2C1_EXT 0x3ff67000
#define ESP32_SDMMC 0x3ff68000
#define ESP32_EMAC 0x3ff69000
#define ESP32_CAN 0x3ff6B000
#define ESP32_PWM1 0x3ff6C000
#define ESP32_I2S1 0x3ff6D000
#define ESP32_UART2 0x3ff6E000
#define ESP32_PWM2 0x3ff6F000
#define ESP32_PWM3 0x3ff70000
#define PERIPHS_SPI_ENCRYPTADDR ESP32_SPI_ENCRYPT

#define NOPS_PER_MS 26643
static inline void delay_us(volatile unsigned long us) {
  spin(us * NOPS_PER_MS / 1000);
}

static inline void delay_ms(volatile unsigned long ms) {
  spin(ms * NOPS_PER_MS);
}

static inline void ledc_enable(void) {
}

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

static inline void soc_init(void) {
}
