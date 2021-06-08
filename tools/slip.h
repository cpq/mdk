#pragma once

#include <stdint.h>

// https://datatracker.ietf.org/doc/html/rfc1055
enum { END = 0300, ESC = 0333, ESC_END = 0334, ESC_ESC = 0335 };

static inline void slip_send(const void *buf, size_t len,
                             void (*fn)(uint8_t, void *), void *arg) {
  const uint8_t *p = buf;
  fn(END, arg);
  for (size_t i = 0; i < len; i++) {
    if (p[i] == END) {
      fn(ESC, arg);
      fn(ESC_END, arg);
    } else if (p[i] == ESC) {
      fn(ESC, arg);
      fn(ESC_ESC, arg);
    } else {
      fn(p[i], arg);
    }
  }
  fn(END, arg);
}

// SLIP state machine
struct slip {
  uint8_t *buf;  // Buffer for the network mode
  size_t size;   // Buffer size
  size_t len;    // Number of currently buffered bytes
  int mode;      // Operation mode. 0 - serial, 1 - network
  uint8_t prev;  // Previously read character
};

// Process incoming byte `c`.
// In serial mode, do nothing, return 1.
// In network mode, append a byte to the `buf` and increment `len`.
// Return size of the buffered packet when switching to serial mode, or 0
static inline size_t slip_recv(uint8_t c, struct slip *slip) {
  size_t res = 0;
  if (slip->mode) {
    if (slip->prev == ESC && c == ESC_END) {
      slip->buf[slip->len++] = END;
    } else if (slip->prev == ESC && c == ESC_ESC) {
      slip->buf[slip->len++] = ESC;
    } else if (c == END) {
      res = slip->len;
    } else if (c != ESC) {
      slip->buf[slip->len++] = c;
    }
    if (slip->len >= slip->size) slip->len = 0;  // Silent overflow
  }
  slip->prev = c;
  // The "END" character flips the mode
  if (c == END) slip->len = 0, slip->mode = !slip->mode;
  return res;
}
