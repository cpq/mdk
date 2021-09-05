CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Os
BINDIR ?= .

all:
	@echo available targets: slipterm esputil test examples

esputil: esputil.c
	$(CC) $(CFLAGS) $? -o $(BINDIR)/$@

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
	rm -rf slipterm esputil *.dSYM *.o
