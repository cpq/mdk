MEMORY {
    irom    (x):    ORIGIN = 0x42000000, LENGTH = 4m
    drom    (r):    ORIGIN = 0x3c000000, LENGTH = 4m
    ram     (rw):   ORIGIN = 0x3fc80000, LENGTH = 320k
    rtc_ram (rx):   ORIGIN = 0x50000000, LENGTH = 8k
}

ENTRY(_start)

SECTIONS {
  .vectors  : { KEEP(*(.vectors)) } > irom
  .text     : { *(.text*)         } > irom
  .rodata   : { *(.rodata*)       } > drom

  .data : {
    _sdata = .;   /* for init_ram() */
    *(.first_data)
    *(.data SORT(.data.*))
    _edata = .;  /* for init_ram() */
  } > ram AT > drom
  _sidata = LOADADDR(.data);

  .bss : {
    _sbss = .;              /* for init_ram() */
    __bss_start__ = _sbss;  /* for libc */
    *(.bss SORT(.bss.*) COMMON)
    _ebss = .;              /* for init_ram() */
    __bss_end__ = _ebss;    /* for libc */
  } > ram

  . = ALIGN(8); /* Make sure that heap pointer is 8-byte aligned */
  end = .;      /* for syscalls.c */
  _end = .;     /* for init_ram() */
  __end__ = .;  /* for libc */

  __global_pointer$ = MIN(_sdata + 0x800, _ebss - 0x800);
}

__absvdi2 = 0x40000764;
__absvsi2 = 0x40000768;
__adddf3 = 0x4000076c;
__addsf3 = 0x40000770;
__addvdi3 = 0x40000774;
__addvsi3 = 0x40000778;
__ashldi3 = 0x4000077c;
__ashrdi3 = 0x40000780;
__bswapdi2 = 0x40000784;
__bswapsi2 = 0x40000788;
__clear_cache = 0x4000078c;
__clrsbdi2 = 0x40000790;
__clrsbsi2 = 0x40000794;
__clzdi2 = 0x40000798;
__clzsi2 = 0x4000079c;
__cmpdi2 = 0x400007a0;
__ctzdi2 = 0x400007a4;
__ctzsi2 = 0x400007a8;
__divdc3 = 0x400007ac;
__divdf3 = 0x400007b0;
__divdi3 = 0x400007b4;
__divsc3 = 0x400007b8;
__divsf3 = 0x400007bc;
__divsi3 = 0x400007c0;
__eqdf2 = 0x400007c4;
__eqsf2 = 0x400007c8;
__extendsfdf2 = 0x400007cc;
__ffsdi2 = 0x400007d0;
__ffssi2 = 0x400007d4;
__fixdfdi = 0x400007d8;
__fixdfsi = 0x400007dc;
__fixsfdi = 0x400007e0;
__fixsfsi = 0x400007e4;
__fixunsdfsi = 0x400007e8;
__fixunssfdi = 0x400007ec;
__fixunssfsi = 0x400007f0;
__floatdidf = 0x400007f4;
__floatdisf = 0x400007f8;
__floatsidf = 0x400007fc;
__floatsisf = 0x40000800;
__floatundidf = 0x40000804;
__floatundisf = 0x40000808;
__floatunsidf = 0x4000080c;
__floatunsisf = 0x40000810;
__gcc_bcmp = 0x40000814;
__gedf2 = 0x40000818;
__gesf2 = 0x4000081c;
__gtdf2 = 0x40000820;
__gtsf2 = 0x40000824;
__ledf2 = 0x40000828;
__lesf2 = 0x4000082c;
__lshrdi3 = 0x40000830;
__ltdf2 = 0x40000834;
__ltsf2 = 0x40000838;
__moddi3 = 0x4000083c;
__modsi3 = 0x40000840;
__muldc3 = 0x40000844;
__muldf3 = 0x40000848;
__muldi3 = 0x4000084c;
__mulsc3 = 0x40000850;
__mulsf3 = 0x40000854;
__mulsi3 = 0x40000858;
__mulvdi3 = 0x4000085c;
__mulvsi3 = 0x40000860;
__nedf2 = 0x40000864;
__negdf2 = 0x40000868;
__negdi2 = 0x4000086c;
__negsf2 = 0x40000870;
__negvdi2 = 0x40000874;
__negvsi2 = 0x40000878;
__nesf2 = 0x4000087c;
__paritysi2 = 0x40000880;
__popcountdi2 = 0x40000884;
__popcountsi2 = 0x40000888;
__powidf2 = 0x4000088c;
__powisf2 = 0x40000890;
__subdf3 = 0x40000894;
__subsf3 = 0x40000898;
__subvdi3 = 0x4000089c;
__subvsi3 = 0x400008a0;
__truncdfsf2 = 0x400008a4;
__ucmpdi2 = 0x400008a8;
__udivdi3 = 0x400008ac;
__udivmoddi4 = 0x400008b0;
__udivsi3 = 0x400008b4;
__udiv_w_sdiv = 0x400008b8;
__umoddi3 = 0x400008bc;
__umodsi3 = 0x400008c0;
__unorddf2 = 0x400008c4;
__unordsf2 = 0x400008c8;


/***************************************
 Group newlib
 ***************************************/

/* Functions */
memset = 0x40000354;
memcpy = 0x40000358;
memmove = 0x4000035c;
memcmp = 0x40000360;
strcpy = 0x40000364;
strncpy = 0x40000368;
strcmp = 0x4000036c;
strncmp = 0x40000370;
strlen = 0x40000374;
strstr = 0x40000378;
bzero = 0x4000037c;
isalnum = 0x40000388;
isalpha = 0x4000038c;
isascii = 0x40000390;
isblank = 0x40000394;
iscntrl = 0x40000398;
isdigit = 0x4000039c;
islower = 0x400003a0;
isgraph = 0x400003a4;
isprint = 0x400003a8;
ispunct = 0x400003ac;
isspace = 0x400003b0;
isupper = 0x400003b4;
toupper = 0x400003b8;
tolower = 0x400003bc;
toascii = 0x400003c0;
memccpy = 0x400003c4;
memchr = 0x400003c8;
memrchr = 0x400003cc;
strcasecmp = 0x400003d0;
strcasestr = 0x400003d4;
strcat = 0x400003d8;
strdup = 0x400003dc;
strchr = 0x400003e0;
strcspn = 0x400003e4;
strcoll = 0x400003e8;
strlcat = 0x400003ec;
strlcpy = 0x400003f0;
strlwr = 0x400003f4;
strncasecmp = 0x400003f8;
strncat = 0x400003fc;
strndup = 0x40000400;
strnlen = 0x40000404;
strrchr = 0x40000408;
strsep = 0x4000040c;
strspn = 0x40000410;
strtok_r = 0x40000414;
strupr = 0x40000418;
longjmp = 0x4000041c;
setjmp = 0x40000420;
abs = 0x40000424;
div = 0x40000428;
labs = 0x4000042c;
ldiv = 0x40000430;
qsort = 0x40000434;
rand_r = 0x40000438;
rand = 0x4000043c;
srand = 0x40000440;
utoa = 0x40000444;
itoa = 0x40000448;
atoi = 0x4000044c;
atol = 0x40000450;
strtol = 0x40000454;
strtoul = 0x40000458;
uart_tx_one_char = 0x40000068;
