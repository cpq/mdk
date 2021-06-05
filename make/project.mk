PROG      ?= firmware
ROOT_PATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= esp32
OBJ_PATH  = ./obj
PORT      ?= /dev/ttyUSB0
ESPTOOL   ?= esptool.py
TOOLCHAIN ?= xtensa-esp32-elf

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

INCLUDES  ?= -I. -I$(ROOT_PATH)/include
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -O3 -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(CFLAGS_EXTRA)
LINKFLAGS ?= $(MCUFLAGS) -T$(ROOT_PATH)/ld/$(ARCH).ld --gc-sections

ifeq "$(ARCH)" "c3"
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32
WARNFLAGS ?= -Wformat-truncation
else 
MCUFLAGS  ?= -mlongcalls -mtext-section-literals
endif

SOURCES += $(wildcard $(ROOT_PATH)/src/*.c)
OBJECTS = $(SOURCES:%.c=$(OBJ_PATH)/%.o) $(OBJ_PATH)/boot.o

build: $(OBJ_PATH)/$(PROG).bin

unix: MCUFLAGS =
unix: OPTFLAGS = -O0 -g3
unix: $(SOURCES)
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) $(SOURCES) -o $(OBJ_PATH)/firmware

$(OBJ_PATH)/%.o: %.c
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/boot.o: $(ROOT_PATH)/boot/boot_$(ARCH).s
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)-as -g --warn --fatal-warnings $(MCUFLAGS) $< -o $@

$(OBJ_PATH)/$(PROG).elf: $(OBJECTS)
	$(TOOLCHAIN)-ld $(OBJECTS) $(LINKFLAGS) -o $@ 
	$(TOOLCHAIN)-size $@

$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	$(ESPTOOL) --chip esp32 elf2image --flash_mode="dio" --flash_freq "40m" --flash_size "4MB" -o $@ $<

flash: $(OBJ_PATH)/$(PROG).bin
	$(ESPTOOL) --chip esp32 --port $(PORT) --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
    0x1000 $(ROOT_PATH)/boot/bootloader_$(ARCH).bin \
    0x8000 $(ROOT_PATH)/boot/partitions.bin \
    0x10000 $?

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)
