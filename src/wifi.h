// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include "soc.h"

#if defined(ESP32C3)
static inline void wifi_get_mac_addr(uint8_t mac[6]) {
  uint32_t a = REG(C3_EFUSE)[17], b = REG(C3_EFUSE)[18];
  mac[0] = (b >> 8) & 255, mac[1] = b & 255, mac[2] = (uint8_t)(a >> 24) & 255;
  mac[3] = (a >> 16) & 255, mac[4] = (a >> 8) & 255, mac[5] = a & 255;
}
#elif defined(ESP32)
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
#else
#error "Ouch"
#endif
