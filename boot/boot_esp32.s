.section .vectors
.word 0x11223344
.word 0x77777777

.section .text
.global _reset
_reset:
  j startup
  j .
