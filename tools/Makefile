CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Os $(EXTRA_CFLAGS)
BINDIR ?= .
CWD ?= $(realpath $(CURDIR))
DOCKER = docker run $(DA) --rm -e Tmp=. -e WINEDEBUG=-all -v $(CWD):$(CWD) -w $(CWD)
SERIAL_PORT ?= /dev/ttyUSB0

all:
	@echo available targets: slipterm esputil test

esputil: esputil.c
	$(CC) $(CFLAGS) $? -o $(BINDIR)/$@

esputil.exe: esputil.c
	$(DOCKER) mdashnet/vc98 wine cl /nologo /W3 /MD /Os $? ws2_32.lib /Fe$@
#	$(DOCKER) mdashnet/vc2017 wine64 cl /nologo /W3 /Os $? /Fe$@

test_esputil_windows: esputil.exe
	ln -fs $(SERIAL_PORT) ~/.wine/dosdevices/com55 && wine $? -p '\\.\COM55' -v info
#	$(DOCKER) -it --device $(SERIAL_PORT) mdashnet/vc98 \
    ln -fs $(SERIAL_PORT) /root/.wine/dosdevices/com3 ; \
    wine $@ -v -p '\\.\COM3' monitor

slipterm: slipterm.c
	$(CC) $(CFLAGS) $? -lpcap -lutil -o $(BINDIR)/$@

clean:
	rm -rf slipterm esputil *.dSYM *.o *.obj _CL*
