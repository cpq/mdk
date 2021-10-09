// Copyright (c) 2021 Cesanta
// All rights reserved

#include <string.h>

size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n] != '\0') n++;
  return n;
}

void *memset(void *buf, int ch, size_t len) {
  unsigned char *s = buf, c = (unsigned char) ch;
  while (len--) *s++ = c;
  return buf;
}

void *memcpy(void *dst, const void *src, size_t len) {
  unsigned char *d = dst, *s = (unsigned char *) src;
  for (size_t i = 0; i < len; i++) d[i] = s[i];
  return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *pa = a, *pb = b;
  while (n-- && *pa == *pb) pa++, pb++;
  return *pa - *pb;
}
