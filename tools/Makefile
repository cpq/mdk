CFLAGS ?= -W -Wall -Wextra -Wundef -Wshadow -Wdouble-promotion -Os -static -s
BINDIR ?= .
CWD ?= $(realpath $(CURDIR))
DOCKER = docker run $(DA) --rm -e Tmp=. -e WINEDEBUG=-all -v $(CWD):$(CWD) -w $(CWD)
SERIAL_PORT ?= /dev/ttyUSB0
COMPILER ?= musl-gcc

all:
	@echo available targets: slipterm esputil test examples

esputil: esputil.c
	$(COMPILER) $(CFLAGS) $? -o $(BINDIR)/$@

esputil.exe: esputil.c
	$(DOCKER) mdashnet/vc98 wine cl /nologo /W3 /Os $? /Fe$@
#	$(DOCKER) mdashnet/vc2017 wine64 cl /nologo /W3 /Os $? /Fe$@

tcc: riscv32-tcc
riscv32-tcc:
	$(COMPILER) tcc/tcc.c -DTCC_TARGET_RISCV32 -DTCC_VERSION=\""0.9.27-mdk\"" $(CFLAGS) -w -O0 -o $@

test_esputil_windows: esputil.exe
	ln -fs $(SERIAL_PORT) ~/.wine/dosdevices/com55 && wine $? -p '\\.\COM55' -v info

slipterm: slipterm.c
	$(COMPILER) $(CFLAGS) $? -lpcap -lutil -o $(BINDIR)/$@

test: unit_test.c
	$(COMPILER) -I../include -W -Wall -g3 $? -o /tmp/t && /tmp/t

examples: examples-build examples-clean

examples-build:
	for d in ../examples/*; do make -C $$d clean build || break; done

examples-clean:
	for d in ../examples/*; do make -C $$d clean ; done

clean: examples-clean
	rm -rf slipterm esputil *.dSYM *.o *.exe *.obj _CL* riscv32-tcc
