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
PROVIDE(memchr = 0x4000c244);
PROVIDE(memcmp = 0x4000c260);
PROVIDE(memcpy = 0x4000c2c8);
PROVIDE(memmove = 0x4000c3c0);
PROVIDE(memrchr = 0x4000c400);
PROVIDE(memset = 0x4000c44c);
PROVIDE(qsort = 0x40056424);
PROVIDE(rand = 0x40001058);
PROVIDE(srand = 0x40001004);
PROVIDE(strcasecmp = 0x400011cc);
PROVIDE(strcasestr = 0x40001210);
PROVIDE(strcat = 0x4000c518);
PROVIDE(strchr = 0x4000c53c);
PROVIDE(strcmp = 0x40001274);
PROVIDE(strcpy = 0x400013ac);
PROVIDE(strcspn = 0x4000c558);
PROVIDE(strdup = 0x4000143c);
PROVIDE(strlen = 0x400014c0);
PROVIDE(strncasecmp = 0x40001550);
PROVIDE(strncat = 0x4000c5c4);
PROVIDE(strncmp = 0x4000c5f4);
PROVIDE(strncpy = 0x400015d4);
PROVIDE(strndup = 0x400016b0);
PROVIDE(strnlen = 0x4000c628);
PROVIDE(strrchr = 0x40001708);
PROVIDE(strsep = 0x40001734);
PROVIDE(strspn = 0x4000c648);
PROVIDE(strstr = 0x4000c674);
PROVIDE(strtol = 0x4005681c);

PROVIDE ( printf = 0x40007d54 );
