.balign 0x100
.global _vectortab
.type _vectortab, @function
_vectortab:
  .option push
  .option norvc
  j _halt     /* exception handler, entry 0 */
  .rept 31
  j _irq      /* 31 identical entries, all pointing to _irq */
  .endr
  .option pop
  .size _vectortab, .-_vectortab


/* On exception, just fall into a busy loop */
.global _halt
.type _halt, @function
_halt:
  j _halt
  .size  _halt, .-_halt

/* On interrupt, return. Make it a weak symbol: allow to redefine */
.global _irq
.weak _irq
.type _irq, @function
_irq:
  mret
  .size _irq, .-_irq
