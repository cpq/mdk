MDK      ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROG     ?= firmware
ARCH     ?= ESP32C3
COMPILER ?= $(MDK)/tools/riscv32-tcc
ESPUTIL  ?= $(MDK)/tools/esputil
BLOFFSET ?= 0

INCLUDES  ?= -I. -I$(MDK)/src -I$(MDK)/src/libc -I$(MDK)/tools/tcc/include -D$(ARCH) -nostdinc
CFLAGS    ?= -W -Wall -Os -nostdinc $(INCLUDES) $(EXTRA_CFLAGS)
LINKFLAGS ?= -nostdlib -static \
             -Wl,-image-base=0x40380400 \
             -Wl,-data-base=0x3fc88000

#             -Wl,--defsym=memset=0x40000354
#             -Wl,--defsym=strlen=0x40000374
#             -Wl,--defsym=memcpy=0x40000358
#             -Wl,--defsym=memcmp=0x40000360

# boot.c must be first, to make _start be an entry point where .text begins
SYSS ?= $(MDK)/src/boot/boot.c $(wildcard $(MDK)/src/*.c)
HDRS += $(wildcard $(MDK)/src/*.h) $(HEADERS)
SRCS ?= $(SYSS) $(SOURCES)

build: $(PROG).bin

#unix: CFLAGS = -W -Wall -Wextra -O0 -g3
unix: SRCS = $(filter-out %.s,$(filter-out $(MDK)/src/malloc.c,$(SRCS)))
unix: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o firmware

$(PROG).elf: $(COMPILER) $(SRCS) $(HDRS)
	$(COMPILER) $(SRCS) $(CFLAGS) $(LINKFLAGS) -o $@

$(PROG).bin: $(ESPUTIL) $(PROG).elf
	$(ESPUTIL) mkbin $(PROG).elf $@

flash: $(ESPUTIL) $(PROG).bin
	$(ESPUTIL) flash $(BLOFFSET) $(PROG).bin

monitor: $(ESPUTIL)
	$(ESPUTIL) monitor

$(ESPUTIL):
	$(MAKE) -C $(MDK)/tools esputil

$(COMPILER):
	$(MAKE) -C $(MDK)/tools tcc

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(PROG) $(PROG).*

# esptool fallback, in case esputil does not work
ESPTOOL   ?= python -m esptool
FLASH_MODE ?= 021f
CHIP_ID ?= 05
CHIP_REV ?= 02
PORT ?= /dev/ttyUSB0
define edit_bin
sed "1s/^\(.\{4\}\).\{4\}/\1$(FLASH_MODE)/; \
     1s/^\(.\{24\}\).\{2\}/\1$(CHIP_ID)/; \
     1s/^\(.\{28\}\).\{2\}/\1$(CIDHIP_REV)/" -
endef

$(OBJ_DIR)/$(PROG)_edit.bin: $(OBJ_DIR)/$(PROG).bin
	xxd -p $< | $(edit_bin) | xxd -p -r - > $@

esptool: $(OBJ_DIR)/$(PROG)_edit.bin
	$(ESPTOOL) -p $(PORT) write_flash $(BLOFFSET) $< 
