PROG      ?= firmware
MDK       ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
ARCH      ?= esp32c3
#TOOLCHAIN ?= riscv64-unknown-elf
ESPUTIL   ?= $(MDK)/tools/esputil

ifeq "$(ARCH)" "esp32c3"
MCUFLAGS  ?= -march=rv32imc -mabi=ilp32
WARNFLAGS ?= -Wformat-truncation
BLOFFSET  ?= 0  # 2nd stage bootloader flash offset
TOOLCHAIN ?= docker run -it --rm -v $(MDK):$(MDK) -w $(CURDIR) mdashnet/riscv riscv-none-elf
LINKFLAGS ?= $(MCUFLAGS) -T$(MDK)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)
else ifeq "$(ARCH)" "esp32"
MCUFLAGS  ?= -mlongcalls -mtext-section-literals
BLOFFSET  ?= 0x1000  # 2nd stage bootloader flash offset
TOOLCHAIN ?= docker run -it --rm -v $(MDK):$(MDK) -w $(CURDIR) espressif/idf xtensa-esp32-elf
LINKFLAGS ?= $(MCUFLAGS) -T$(MDK)/make/$(ARCH).ld -nostdlib -nostartfiles -Wl,--gc-sections $(EXTRA_LINKFLAGS)
else
OPTFLAGS = -O0 -g3
endif

# -g3 pulls enums and defines into the debug info for GDB
# -ffunction-sections -fdata-sections, -Wl,--gc-sections remove unused code
# strict WARNFLAGS protect from stupid mistakes

INCLUDES  ?= -I. -I$(MDK)/src -I$(MDK)/src/$(ARCH) -D$(ARCH)
WARNFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -fno-common -Wconversion
OPTFLAGS  ?= -Os -g3 -ffunction-sections -fdata-sections
CFLAGS    ?= $(WARNFLAGS) $(OPTFLAGS) $(MCUFLAGS) $(INCLUDES) $(EXTRA_CFLAGS)

#SOURCES += $(MDK)/src/boot/boot_$(ARCH).s
SOURCES += $(wildcard $(MDK)/src/*.c) $(wildcard $(MDK)/src/$(ARCH)/*.c)
HEADERS += $(wildcard $(MDK)/src/*.h) $(wildcard $(MDK)/src/$(ARCH)/*.h)

build: $(PROG).bin

$(PROG).elf: $(SOURCES) $(HEADERS)
	$(TOOLCHAIN)-gcc  $(CFLAGS) $(SOURCES) $(LINKFLAGS) -o $@
	$(TOOLCHAIN)-size $@

$(PROG).bin: $(PROG).elf $(ESPUTIL)
	$(ESPUTIL) mkbin $(PROG).elf $@

flash: $(PROG).bin $(ESPUTIL)
	$(ESPUTIL) flash $(BLOFFSET) $(PROG).bin

monitor: $(ESPUTIL)
	$(ESPUTIL) monitor

$(ESPUTIL): $(MDK)/tools/esputil.c
	make -C $(MDK)/tools esputil

clean:
	@rm -rf *.{bin,elf,map,lst,tgz,zip,hex} $(PROG)*

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

$(PROG)_edit.bin: $(PROG).bin
	xxd -p $< | $(edit_bin) | xxd -p -r - > $@

esptool: $(PROG)_edit.bin
	$(ESPTOOL) -p $(PORT) write_flash $(BLOFFSET) $< 
