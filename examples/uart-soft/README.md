# Soft UART echo example

This firmware forwards UART0 to the soft UART attached to pins 4,5.
Wire an external USB TTL converter to pins 4 (RX) and 5 (TX).
Build and flash the firmware:

```sh
$ make clean build flash monitor
```

Type something. You should see characters you've typed on an external
converter - and vica versa.
