PROG      ?= firmware
ROOT_PATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= c3
OBJ_PATH  = ./obj
PORT      ?= /dev/ttyUSB0
ESPTOOL   ?= esptool.py
TOOLCHAIN ?= riscv32-esp-elf
SLIPTERM  ?= $(ROOT_PATH)/tools/slipterm

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

DEFS      ?=
INCLUDES  ?= -I. -I$(ROOT_PATH)/include
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -O3 -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(DEFS) $(PROJFLAGS) $(CFLAGS_EXTRA)
LINKFLAGS ?= $(MCUFLAGS) -T$(ROOT_PATH)/ld/$(ARCH).ld -nostdlib -nostartfiles #--gc-sections

ifeq "$(ARCH)" "c3"
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32 -DESP32C3
WARNFLAGS ?= -Wformat-truncation
else 
MCUFLAGS  ?= -mlongcalls -mtext-section-literals -DESP32
endif

SOURCES += $(ROOT_PATH)/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(ROOT_PATH)/src/*.c)
OBJECTS = $(SOURCES:%.c=$(OBJ_PATH)/%.o) #$(OBJ_PATH)/boot.o

build: $(OBJ_PATH)/$(PROG).bin

unix: MCUFLAGS =
unix: OPTFLAGS = -O0 -g3
unix: $(SOURCES)
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) $(filter-out $(ROOT_PATH)/src/malloc.c,$(SOURCES)) -o $(OBJ_PATH)/firmware

$(OBJ_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/%.o: %.s
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/$(PROG).elf: $(OBJECTS)
	$(TOOLCHAIN)-gcc -Xlinker $(OBJECTS) $(LINKFLAGS) -o $@
	$(TOOLCHAIN)-size $@

$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	$(ESPTOOL) --chip esp32c3 elf2image -o $@ $<

flash: $(OBJ_PATH)/$(PROG).bin
	$(ESPTOOL) --chip esp32c3 --port $(PORT) --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    0x00000 $(ROOT_PATH)/boot/bootloader_$(ARCH).bin \
    0x08000 $(ROOT_PATH)/boot/partitions.bin \
    0x10000 $?

monitor:
	python3 $$(find ~/.espressif/ -name miniterm.py | head -1) $(PORT) 115200

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)
