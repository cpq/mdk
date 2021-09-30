CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Os
BINDIR ?= .
CWD ?= $(realpath $(CURDIR))
DOCKER = docker run $(DA) --rm -e WINEDEBUG=-all -v $(CWD):$(CWD) -w $(CWD)

all:
	@echo available targets: slipterm esputil test examples

esputil: esputil.c
	$(CC) $(CFLAGS) $? -o $(BINDIR)/$@

esputil.exe: esputil.c
	$(DOCKER) mdashnet/vc98 wine cl /nologo /W3 /Os $? /Fe$@
	$(DOCKER) mdashnet/vc98 wine $@ -v -p COM33 info
#	$(DOCKER) mdashnet/mingw i686-w64-mingw32-gcc $(CFLAGS) $? -o $@

slipterm: slipterm.c
	$(CC) $(CFLAGS) $? -lpcap -lutil -o $(BINDIR)/$@

test: unit_test.c
	$(CC) -I../include -W -Wall -g3 $? -o /tmp/t && /tmp/t

examples: examples-build examples-clean

examples-build:
	for d in ../examples/*; do make -C $$d clean build || break; done

examples-clean:
	for d in ../examples/*; do make -C $$d clean ; done

clean: examples-clean
	rm -rf slipterm esputil *.dSYM *.o *.exe *.obj
