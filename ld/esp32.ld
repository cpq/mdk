/* esp32_technical_reference_manual_en.pdf 1.3.2 */
MEMORY {
  drom   (rw)   : ORIGIN = 0X3ff90000, LENGTH = 64k
  irom   (rwx)  : ORIGIN = 0x40000000, LENGTH = 384k

  iram   (rwx)  : ORIGIN = 0x40080000, LENGTH = 128k
  dram   (rw)   : ORIGIN = 0x3ffae000, LENGTH = 328k

  dflash (rw)   : ORIGIN = 0X3f400000, LENGTH = 1024k
  psram  (rw)   : ORIGIN = 0X3f800000, LENGTH = 1024k
  iflash (rwx)  : ORIGIN = 0X400c2000, LENGTH = 11512k
}

ENTRY(_reset)

SECTIONS {
  .vectors  : { KEEP(*(.vectors))   } > iram
  .text     : { *(.text*) *(.text)  } > iram

  .data : {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.rodata)
    *(.rodata*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    . = ALIGN(4);
    _edata = .;
  } > dram

  .bss : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > dram

  . = ALIGN(4);
  PROVIDE(end = .);
  PROVIDE(_end = .);

  /*
  /DISCARD/ : { *(.debug) *(.debug*) *(.xtensa.*) *(.comment) }
  */
}

/* See .ld files in $IDF_PATH/components/esp_rom */
PROVIDE(uart_rx_one_char = 0x400092d0);
PROVIDE(uart_tx_one_char = 0x40009200);

PROVIDE(vsnprintf = 0x40056a68);
PROVIDE(snprintf = 0x40056b4c);
PROVIDE(strcpy = 0x400013ac);
PROVIDE(strlen = 0x400014c0);
PROVIDE(memchr = 0x4000c244);
PROVIDE(memcmp = 0x4000c260);
PROVIDE(memcpy = 0x4000c2c8);
PROVIDE(memmove = 0x4000c3c0);
PROVIDE(memrchr = 0x4000c400);
PROVIDE(memset = 0x4000c44c);
