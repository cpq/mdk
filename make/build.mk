PROG      ?= firmware
ROOT_PATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= c3
OBJ_PATH  = ./build
PORT      ?= /dev/ttyUSB0
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
BLOFFSET  ?= 0  # 2nd stage bootloader flash offset
else 
MCUFLAGS  ?= -mlongcalls -mtext-section-literals -DESP32
BLOFFSET  ?= 0x1000  # 2nd stage bootloader flash offset
endif

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

# Extract section from the .elf file. Save offset, then size, then content
# TODO(cpq): make esputil generate the code
define savesection
	$(TOOLCHAIN)-objcopy -O binary --only-section $3 $1 $(OBJ_PATH)/.tmp.bin
	$(TOOLCHAIN)-objdump -h $1 | grep $3 | tr -s ' ' | cut -d ' ' -f 5 | xxd -r -p | od -An -X | xxd -r -p >> $2
	$(TOOLCHAIN)-objdump -h $1 | grep $3 | tr -s ' ' | cut -d ' ' -f 4 | xxd -r -p | od -An -X | xxd -r -p >> $2
	cat $(OBJ_PATH)/.tmp.bin >> $2
endef

# Pad file $1 to a given alignment $2
# NUM = ALIGN - (FILE_SIZE % ALIGN); head -c NUM /dev/zero >> FILE
define addpadding
	head -c $$(echo "$2-($$(ls -l $1 | tr -s ' ' | cut -d ' ' -f5) % 16)"  | bc) /dev/zero >> $1
endef

# Generate flashable .bin image from the .elf file
$(OBJ_PATH)/$(PROG).bin: $(OBJ_PATH)/$(PROG).elf
	echo e9030000 | xxd -r -p > $@ # Image header, 3 sections, flash params = 0
	$(TOOLCHAIN)-nm $< | grep 'T _reset' | cut -f1 -dT | xxd -r -p | od -An -X | xxd -r -p >> $@ # Entry point address, _reset. Using od for little endian
	echo ee000000000000000000000000000001 | xxd -r -p >> $@ # What's that ??
	$(call savesection,$<,$@,.data)
	$(call savesection,$<,$@,.text)
	$(call savesection,$<,$@,.rodata)
	$(call addpadding,$@,16)

flash: $(OBJ_PATH)/$(PROG).bin $(ESPUTIL)
	$(ESPUTIL) -p $(PORT) flash $(BLOFFSET) $(OBJ_PATH)/$(PROG).bin

monitor: $(ESPUTIL)
	$(ESPUTIL) -p $(PORT) monitor

$(ESPUTIL): $(ROOT_PATH)/tools/esputil.c
	make -C $(ROOT_PATH)/tools esputil

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(OBJ_PATH)
