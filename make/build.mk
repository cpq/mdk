MDK      ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROG     ?= firmware
ARCH     ?= ESP32C3
COMPILER ?= riscv32-tcc
ESPUTIL  ?= esputil
OBJ_DIR  ?= ./build

DEFS      ?=
INCLUDES  ?= -I. -I$(MDK)/src -I$(MDK)/src/libc -I$(MDK)/tools/tcc/include -D$(ARCH) -nostdinc
CFLAGS    ?= -W -Wall -O2 -nostdinc $(INCLUDES) $(DEFS) $(EXTRA_CFLAGS)
LINKFLAGS ?= -nostdlib \
             -Wl,--defsym=memset=0x40000354 \
             -Wl,--defsym=strlen=0x40000374 \
             -Wl,--defsym=memcpy=0x40000358 \
             -Wl,--defsym=memcmp=0x40000360

#SOURCES += $(MDK)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(MDK)/src/*.c)
HEADERS += $(wildcard $(MDK)/src/*.h)

build: $(OBJ_DIR)/$(PROG).bin

#unix: CFLAGS = -W -Wall -Wextra -O0 -g3
unix: SRCS = $(filter-out %.s,$(filter-out $(MDK)/src/malloc.c,$(SOURCES)))
unix: $(SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(OBJ_DIR)/firmware

$(OBJ_DIR)/$(PROG).elf: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(COMPILER) $(SOURCES) $(CFLAGS) $(LINKFLAGS) -o $@

# elf_section_load_address FILE,SECTION_NAME
elf_section_load_address = $(shell $(TOOLCHAIN)-objdump -h $1 | grep -F $2 | tr -s ' ' | cut -d ' ' -f 5)

# elf_symbol_address FILE,SYMBOL
elf_entry_point_address = $(shell $(TOOLCHAIN)-nm $1 | grep 'T $2' | cut -f1 -dT)

$(OBJ_DIR)/$(PROG).bin:
$(OBJ_DIR)/$(PROG).bin: $(OBJ_DIR)/$(PROG).elf
	$(TOOLCHAIN)-objcopy -O binary --only-section .text $< $(OBJ_DIR)/.text.bin
	$(TOOLCHAIN)-objcopy -O binary --only-section .data $< $(OBJ_DIR)/.data.bin
	$(ESPUTIL) mkbin $@ $(call elf_entry_point_address,$<,_reset) $(call elf_section_load_address,$<,.data) $(OBJ_DIR)/.data.bin $(call elf_section_load_address,$<,.text) $(OBJ_DIR)/.text.bin

flash: $(OBJ_DIR)/$(PROG).bin
	$(ESPUTIL) flash $(BLOFFSET) $(OBJ_DIR)/$(PROG).bin

monitor:
	$(ESPUTIL) monitor

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_DIR)

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
