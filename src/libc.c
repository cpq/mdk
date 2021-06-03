//  Portions (Ok, most of weblibc) is basically stolen from libmusl.
//  weblibc is under the same, an MIT license.
//
// All non-MUSL content is Copyright <>< 2020 Charles Lohr (under the
//  below license)
//
// ----------------------------------------------------------------------
// Copyright © 2005-2020 Rich Felker, et al.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <string.h>

size_t strlen(const char *s) {
  const char *a = s;
  while (*s) s++;
  return (size_t)(s - a);
}

size_t strnlen(const char *s, size_t n) {
  const char *p = memchr(s, 0, n);
  return p ? (size_t)(p - s) : n;
}

void *memset(void *dest, int c, size_t n) {
  unsigned char *s = dest, v = (unsigned char) c;
  while (n--) *s++ = v;
  return dest;
}

void *memcpy(void *dst, const void *src, size_t len) {
  unsigned char *d = dst, *s = (unsigned char *) src;
  for (size_t i = 0; i < len; i++) d[i] = s[i];
  return dst;
}

char *strcpy(char *dst, const char *src) {
  for (size_t i = 0; src[i] != '\0'; i++) dst[i] = src[i];
  return dst;
}

char *strncpy(char *dst, const char *src, size_t len) {
  for (size_t i = 0; i < len && src[i] != '\0'; i++) dst[i] = src[i];
  return dst;
}

int strcmp(const char *l, const char *r) {
  while (*l == *r && *l) l++, r++;
  return *(unsigned char *) l - *(unsigned char *) r;
}

int strncmp(const char *_l, const char *_r, size_t n) {
  const unsigned char *l = (void *) _l, *r = (void *) _r;
  if (!n--) return 0;
  while (*l && *r && n && *l == *r) l++, r++, n--;
  return *l - *r;
}
