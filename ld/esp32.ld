/* esp32_technical_reference_manual_en.pdf 1.3.2 */
MEMORY {
  drom   (rw)   : ORIGIN = 0X3ff90000, LENGTH = 64k
  irom   (rwx)  : ORIGIN = 0x40000000, LENGTH = 384k

  cache0 (rwx)  : ORIGIN = 0x40070000, LENGTH = 32k
  cache1 (rwx)  : ORIGIN = 0x40078000, LENGTH = 32k
  iram   (rwx)  : ORIGIN = 0x40080400, LENGTH = 127k  /* First 1k is vectors */
  dram   (rw)   : ORIGIN = 0x3ffb0000, LENGTH = 320k

  dflash (rw)   : ORIGIN = 0X3f400000, LENGTH = 1024k
  psram  (rw)   : ORIGIN = 0X3f800000, LENGTH = 1024k
  iflash (rwx)  : ORIGIN = 0X400c2000, LENGTH = 11512k
}

_eram = ORIGIN(dram) + LENGTH(dram);
ENTRY(_reset)

SECTIONS {
  /* .vectors  : { KEEP(*(.vectors))   } > iram */
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
