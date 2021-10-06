MDK      ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROG     ?= firmware
ARCH     ?= ESP32C3
COMPILER ?= riscv32-tcc
OBJ_DIR  ?= ./build
ESPUTIL  ?= esputil

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

DEFS      ?=
INCLUDES  ?= -I. -I$(MDK)/src -I$(MDK)/src/libc -I$(MDK)/tools/tcc/include -D$(ARCH) -nostdinc
WARNFLAGS ?= -W -Wall
OPTFLAGS  ?= -O2
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(DEFS) $(EXTRA_CFLAGS)
LINKFLAGS ?= $(MCUFLAGS) -T$(MDK)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)

SOURCES += $(MDK)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(MDK)/src/*.c)
HEADERS += $(wildcard $(MDK)/src/*.h)
O1       = $(SOURCES:%.c=$(OBJ_DIR)/%.o)
O2       = $(O1:%.cpp=$(OBJ_DIR)/%.o)
OBJECTS  = $(O2:%.s=$(OBJ_DIR)/%.o)

build: $(OBJ_DIR)/$(PROG).bin
$(OBJECTS): $(HEADERS)

unix: MCUFLAGS =
unix: OPTFLAGS = -O0 -g3
unix: SRCS = $(filter-out %.s,$(filter-out $(MDK)/src/malloc.c,$(SOURCES)))
unix: $(SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(OBJ_DIR)/firmware

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILER) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(COMPILER) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/$(PROG).elf: $(OBJECTS)
	$(COMPILER) -Xlinker $(OBJECTS) $(LINKFLAGS) -o $@
#	$(COMPILER)-size $@

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
