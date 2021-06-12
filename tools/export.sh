#!/bin/bash

if [ "$#" -le 0 ]; then
	echo "Usage:" >&2
	echo "" >&2
	echo " . tools/export.sh [arch] [chip rev (optional)]" >&2
	echo "" >&2
	echo "where arch is:" >&2
	ls boot/boot_* | cut -c 11- | cut -d. -f1
else
	if [ $1 == "esp32" ]; then
		export TOOLCHAIN=xtensa-esp32-elf
	else
		export TOOLCHAIN=riscv32-esp-elf
	fi
	export ARCH=$1
	export ESPTOOL=/path/to/esptool.py 
	export ESPTOOL=/home/cnlohr/esp/esp-idf/components/esptool_py/esptool/esptool.py
	export PORT=/dev/ttyUSB0
	export PATH=$PATH:$(dirname $(find ~/.espressif -type f -name $TOOLCHAIN-gcc))
	export CHIP_REV=$2
	$TOOLCHAIN-gcc -v
fi

