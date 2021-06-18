# Soft UART echo example

This firmware forwards UART0 to UART1.
Build and flash the firmware:

```sh
$ make clean build flash monitor
```

Wire an external USB TTL converter to pins 2 (TX) and 3 (RX).
In the second terminal, start a terminal emulator.

Type something in the 1st terminal. You should see characters you've
typed on the 2nd terminal, and vice versa.
