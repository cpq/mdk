PROG      ?= firmware
MDK       ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= ESP32C3
#TOOLCHAIN ?= riscv64-unknown-elf
TOOLCHAIN ?= docker run -it --rm -v $(MDK):$(MDK) -w $(CURDIR) mdashnet/riscv riscv-none-elf
OBJ_PATH  ?= ./build
ESPUTIL   ?= $(MDK)/tools/esputil

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

DEFS      ?=
INCLUDES  ?= -I. -I$(MDK)/src -D$(ARCH)
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -Os -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(DEFS) $(EXTRA_CFLAGS)
LINKFLAGS ?= $(MCUFLAGS) -T$(MDK)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)

ifeq "$(ARCH)" "ESP32C3"
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32
WARNFLAGS ?= -Wformat-truncation
BLOFFSET  ?= 0  # 2nd stage bootloader flash offset
else 
MCUFLAGS  ?= -mlongcalls -mtext-section-literals
BLOFFSET  ?= 0x1000  # 2nd stage bootloader flash offset
endif

SOURCES += $(MDK)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(MDK)/src/*.c)
HEADERS += $(wildcard $(MDK)/src/*.h)
O1       = $(SOURCES:%.c=$(OBJ_PATH)/%.o)
O2       = $(O1:%.cpp=$(OBJ_PATH)/%.o)
OBJECTS  = $(O2:%.s=$(OBJ_PATH)/%.o)

build: $(OBJ_PATH)/$(PROG).bin
$(OBJECTS): $(HEADERS)

$(ESPUTIL): $(MDK)/tools/esputil.c
	make -C $(MDK)/tools esputil

unix: MCUFLAGS =
unix: OPTFLAGS = -O0 -g3
unix: SRCS = $(filter-out %.s,$(filter-out $(MDK)/src/malloc.c,$(SOURCES)))
unix: $(SRCS)
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) $(SRCS) -o $(OBJ_PATH)/firmware

$(OBJ_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-g++ $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/$(PROG).elf: $(OBJECTS)
	$(TOOLCHAIN)-gcc -Xlinker $(OBJECTS) $(LINKFLAGS) -o $@
	$(TOOLCHAIN)-size $@

# elf_section_load_address FILE,SECTION_NAME
elf_section_load_address = $(shell $(TOOLCHAIN)-objdump -h $1 | grep -F $2 | tr -s ' ' | cut -d ' ' -f 5)

# elf_symbol_address FILE,SYMBOL
elf_entry_point_address = $(shell $(TOOLCHAIN)-nm $1 | grep 'T $2' | cut -f1 -dT)

$(OBJ_PATH)/$(PROG).bin: $(ESPUTIL)
$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	$(TOOLCHAIN)-objcopy -O binary --only-section .text $< $(OBJ_PATH)/.text.bin
	$(TOOLCHAIN)-objcopy -O binary --only-section .data $< $(OBJ_PATH)/.data.bin
	$(ESPUTIL) mkbin $@ $(call elf_entry_point_address,$<,_reset) $(call elf_section_load_address,$<,.data) $(OBJ_PATH)/.data.bin $(call elf_section_load_address,$<,.text) $(OBJ_PATH)/.text.bin

flash: $(OBJ_PATH)/$(PROG).bin $(ESPUTIL)
	$(ESPUTIL) flash $(BLOFFSET) $(OBJ_PATH)/$(PROG).bin

monitor: $(ESPUTIL)
	$(ESPUTIL) monitor

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)

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

$(OBJ_PATH)/$(PROG)_edit.bin: $(OBJ_PATH)/$(PROG).bin
	xxd -p $< | $(edit_bin) | xxd -p -r - > $@

esptool: $(OBJ_PATH)/$(PROG)_edit.bin
	$(ESPTOOL) -p $(PORT) write_flash $(BLOFFSET) $< 
