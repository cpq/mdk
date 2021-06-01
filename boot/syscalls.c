#include <sdk.h>

extern int main(void);

// Initialise memory and other low level stuff, and call main()
void _start(void) {
  extern long _sbss, _ebss;
  memset(&_sbss, 0, (size_t)(((char *) &_ebss - (char *) &_sbss)));
  main();
}
