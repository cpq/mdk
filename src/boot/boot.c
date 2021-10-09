// Firmware entry point
void _start(void) {
  extern void begin(void);
  begin();
  for (;;) asm("nop");
}
