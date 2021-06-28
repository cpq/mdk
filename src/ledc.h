// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include "soc.h"

#if defined(ESP32C3)
static inline void ledc_enable(void) {
  REG(C3_SYSTEM)[4] |= BIT(11);                // Enable LEDC clock
  REG(C3_LEDC)[52] = 1U | BIT(31);             // Use APB clock, 15.3.2.1
  REG(C3_LEDC)[40] = 15U | BIT(12) | BIT(25);  // Timer 0 overflow 14-bit
  REG(C3_LEDC)[0] = BIT(2) | BIT(4);           // Chan 0 config
  REG(C3_LEDC)[1] = 4096;                      // Chan 0 Hpoint
  REG(C3_LEDC)[2] = 1 << 4;                    // Chan 0 Lpoint
}
#elif defined(ESP32)
static inline void ledc_enable(void) {
}
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
static inline void ledc_enable(void) {
}
#else
#error "Ouch"
#endif
