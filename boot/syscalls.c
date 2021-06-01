#include <sdk.h>

extern int main(void);

// Initialise memory and other low level stuff, and call main()
void _start(void) {
  extern char _sbss, _ebss;
  memset(&_sbss, 0, (size_t)(&_ebss - &_sbss));

  // extern uint32_t _sdata, _edata, _sidata;
  // memcpy(&_sdata, &_sidata, (size_t)(((char *) &_edata - (char *) &_sdata)));

  main();
}
