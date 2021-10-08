MDK      ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROG     ?= firmware
ARCH     ?= ESP32C3
COMPILER ?= riscv32-tcc
ESPUTIL  ?= esputil

INCLUDES  ?= -I. -I$(MDK)/src -I$(MDK)/src/libc -I$(MDK)/tools/tcc/include -D$(ARCH) -nostdinc
CFLAGS    ?= -W -Wall -O2 -nostdinc $(INCLUDES) $(EXTRA_CFLAGS)
LINKFLAGS ?= -nostdlib -static \
             -Wl,-image-base=0x40380400 \
             -Wl,--defsym=memset=0x40000354 \
             -Wl,--defsym=strlen=0x40000374 \
             -Wl,--defsym=memcpy=0x40000358 \
             -Wl,--defsym=memcmp=0x40000360

#SOURCES += $(MDK)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(MDK)/src/*.c)
HEADERS += $(wildcard $(MDK)/src/*.h)

build: $(PROG).bin

#unix: CFLAGS = -W -Wall -Wextra -O0 -g3
unix: SOURCES = $(filter-out %.s,$(filter-out $(MDK)/src/malloc.c,$(SOURCES)))
unix: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o firmware

$(PROG).elf: $(SOURCES) $(HEADERS)
	$(COMPILER) $(SOURCES) $(CFLAGS) $(LINKFLAGS) -o $@

$(PROG).bin: $(PROG).elf
	$(ESPUTIL) mkbin $? $@

flash: $(PROG).bin
	$(ESPUTIL) flash $(BLOFFSET) $?

monitor:
	$(ESPUTIL) monitor

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
