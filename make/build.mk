PROG      ?= firmware
ROOT_PATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= ESP32C3
OBJ_PATH  = ./build
PORT      ?= /dev/ttyUSB0
ESPUTIL   ?= $(ROOT_PATH)/tools/esputil

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

DEFS      ?=
INCLUDES  ?= -I. -I$(ROOT_PATH)/src -D$(ARCH)
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -Os -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(DEFS) $(EXTRA_CFLAGS)
LINKFLAGS ?= $(MCUFLAGS) -T$(ROOT_PATH)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)

ifeq "$(ARCH)" "ESP32C3"
TOOLCHAIN ?= riscv64-unknown-elf
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32
WARNFLAGS ?= -Wformat-truncation
BLOFFSET  ?= 0  # 2nd stage bootloader flash offset
else 
TOOLCHAIN ?= xtensa-esp32-elf
MCUFLAGS  ?= -mlongcalls -mtext-section-literals
BLOFFSET  ?= 0x1000  # 2nd stage bootloader flash offset
endif

SOURCES += $(ROOT_PATH)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(ROOT_PATH)/src/*.c)
HEADERS += $(wildcard $(ROOT_PATH)/src/*.h)
_BJECTS = $(SOURCES:%.c=$(OBJ_PATH)/%.o)
OBJECTS = $(_BJECTS:%.cpp=$(OBJ_PATH)/%.o)

build: $(OBJ_PATH)/$(PROG).bin
$(OBJECTS): $(HEADERS)

unix: MCUFLAGS =
unix: OPTFLAGS = -O0 -g3
unix: SRCS = $(filter-out %.s,$(filter-out $(ROOT_PATH)/src/malloc.c,$(SOURCES)))
unix: $(SRCS)
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) $(SRCS) -o $(OBJ_PATH)/firmware

$(OBJ_PATH)/%.o: %.c $(wildcard $(ROOT_PATH)/include/%.h)
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
elf_section_load_address = $(shell $(TOOLCHAIN)-objdump -h $1 | grep $2 | tr -s ' ' | cut -d ' ' -f 5)

# elf_symbol_address FILE,SYMBOL
elf_entry_point_address = $(shell $(TOOLCHAIN)-nm $1 | grep 'T $2' | cut -f1 -dT)

$(OBJ_PATH)/$(PROG).bin: $(ESPUTIL)
$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	$(TOOLCHAIN)-objcopy -O binary --only-section .text $< $(OBJ_PATH)/.text.bin
	$(TOOLCHAIN)-objcopy -O binary --only-section .data $< $(OBJ_PATH)/.data.bin
	$(ESPUTIL) mkbin $@ $(call elf_entry_point_address,$<,_reset) $(call elf_section_load_address,$<,.data) $(OBJ_PATH)/.data.bin $(call elf_section_load_address,$<,.text) $(OBJ_PATH)/.text.bin

flash: $(OBJ_PATH)/$(PROG).bin $(ESPUTIL)
	$(ESPUTIL) -p $(PORT) flash $(BLOFFSET) $(OBJ_PATH)/$(PROG).bin

monitor: $(ESPUTIL)
	$(ESPUTIL) -p $(PORT) monitor

$(ESPUTIL): $(ROOT_PATH)/tools/esputil.c
	make -C $(ROOT_PATH)/tools esputil

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)
