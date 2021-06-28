// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#if defined(ESP32C3)
static inline void wifi_mac(uint8_t mac[6]) {
  // PROVIDE(wifi_get_macaddr = 0x40001874);
  for (int i = 0; i < 6; i++) mac[i] = 0;
  ((void (*)(uint8_t[6]))(uintptr_t) 0x40001874)(mac);
}
#elif defined(ESP32)
#elif defined(__unix) || defined(__unix__) || defined(__APPLE__)
#else
#error "Ouch"
#endif
