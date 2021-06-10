#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/net.c"

int main(void) {
  uint8_t hdr[] = {0x45, 0, 0,    0x54, 0x4, 0x4b, 0,    0,    0x40, 1,
                   0,    0, 0xc0, 0xa8, 0,   0x3b, 0xc0, 0xa8, 0,    7};
  assert(ipcsum(hdr, hdr + sizeof(hdr)) == 0xcbf4);
  // printf("%hx\n", ipcsum(hdr, hdr + sizeof(hdr)));  // f4cb
  return 0;
}
