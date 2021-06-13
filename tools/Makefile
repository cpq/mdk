all: slipterm

slipterm: slipterm.c
	$(CC) -W -Wall -Wextra -Os $? -lpcap -lutil -o $@

slipterm-linux: slipterm
slipterm-linux: CC = docker run -v $(CURDIR):$(CURDIR) -w $(CURDIR) mdashnet/cc2 cc

test: unit_test.c ../src/log.c ../src/init.c
	$(CC) -I../include -W -Wall -g3 $? -o /tmp/t && /tmp/t

clean:
	rm -rf slipterm *.dSYM *.o
