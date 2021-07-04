CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Os

all:
	@echo available targets: slipterm esputil test examples

esputil: esputil.c
	$(CC) $(CFLAGS) $? -o $@

slipterm: slipterm.c
	$(CC) $(CFLAGS) $? -lpcap -lutil -o $@

test: unit_test.c
	$(CC) -I../include -W -Wall -g3 $? -o /tmp/t && /tmp/t

examples:
	for d in ../examples/*; do make -C $$d clean build || break; done
	for d in ../examples/*; do make -C $$d clean ; done

clean:
	rm -rf slipterm esputil *.dSYM *.o
