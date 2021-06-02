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

#include <stdlib.h>
#include <string.h>

struct block {
  size_t size;
  struct block *next;
};

static struct block s_blocks = {0, 0};
extern void *sbrk(int);

void *malloc(size_t size) {
  const size_t align_to = 16;
  struct block *block = s_blocks.next, **head = &s_blocks.next;
  size = (size + sizeof(size_t) + (align_to - 1)) & ~(align_to - 1);
  while (block != 0) {
    if (block->size >= size) {
      *head = block->next;
      return ((char *) block) + sizeof(size_t);
    }
    head = &block->next;
    block = block->next;
  }

  block = (struct block *) sbrk((int) size);
  if (block == NULL) return block;
  block->size = size;

  return ((char *) block) + sizeof(size_t);
}

void free(void *ptr) {
  struct block *block = (struct block *) (((char *) ptr) - sizeof(size_t));
  block->next = s_blocks.next;
  s_blocks.next = block;
}

void *calloc(size_t nmemb, size_t size) {
  size_t len = nmemb * size;
  void *ret = malloc(len);
  if (ret) memset(ret, 0, len);
  return ret;
}

void *realloc(void *ptr, size_t size) {
  struct block *block = (struct block *) (((char *) ptr) - sizeof(size_t));
  size_t osize = block->size;
  if (size < osize) return ptr;  // We do a dumb malloc thing here.
  void *p = malloc(size);
  size_t less = size < osize ? size : osize;
  memcpy(p, ptr, less);
  free(ptr);
  return p;
}
