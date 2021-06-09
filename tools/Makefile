all: slipterm

slipterm: slipterm.c
	$(CC) -W -Wall -O2 -g3 $? -lpcap -o $@

test: unit_test.c ../src/log.c ../src/init.c
	$(CC) -I../include -W -Wall -g3 $? -o /tmp/t && /tmp/t

clean:
	rm -rf slipterm *.dSYM *.o
