PROG      ?= firmware
ROOT_PATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= c3
OBJ_PATH  = ./build
PORT      ?= /dev/ttyUSB0
ESPTOOL   ?= esptool.py
ESPUTIL   ?= $(ROOT_PATH)/tools/esputil
TOOLCHAIN ?= riscv64-unknown-elf

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

DEFS      ?=
INCLUDES  ?= -I. -I$(ROOT_PATH)/src
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -Os -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(DEFS) $(EXTRA_CFLAGS)
LINKFLAGS ?= $(MCUFLAGS) -T$(ROOT_PATH)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)

ifeq "$(ARCH)" "c3"
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32 -DESP32C3
WARNFLAGS ?= -Wformat-truncation
BLOFFSET  ?= 0
CHIP      ?= esp32c3
else 
MCUFLAGS  ?= -mlongcalls -mtext-section-literals -DESP32
BLOFFSET  ?= 0x1000
CHIP      ?= esp32
endif

# Most chips don't need a specific rev.
CHIP_REV?=
FLASH_BAUD?=115200

SOURCES += $(ROOT_PATH)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(ROOT_PATH)/src/*.c)
_BJECTS = $(SOURCES:%.c=$(OBJ_PATH)/%.o)
OBJECTS = $(_BJECTS:%.cpp=$(OBJ_PATH)/%.o)

build: $(OBJ_PATH)/$(PROG).bin

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

$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	$(ESPTOOL) --chip $(CHIP) elf2image -o $@ $<
#	$(TOOLCHAIN)-objcopy -O binary $< $@

flash: $(OBJ_PATH)/$(PROG).bin
	$(ESPTOOL) --chip $(CHIP) --port $(PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect $(BLOFFSET) $?

monitor:
	$(ESPUTIL) -p $(PORT) monitor

$(ESPUTIL): $(ROOT_PATH)/tools/esputil.c
	make -C $(ROOT_PATH)/tools esputil

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)
