// Copyright (c) 2012-2021 Charles Lohr, MIT license
// All rights reserved

#include <cnip.h>

#if 1
#include <sdk.h>
#define DEBUG(fmt, ...) sdk_log(fmt, __VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

void cn_mac_in(void *buf, size_t len) {
  DEBUG("got frame %u bytes", len);
  (void) buf;
  (void) len;
}

void cn_poll(unsigned long now_ms) {
  (void) now_ms;
}
