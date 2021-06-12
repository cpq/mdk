# Software timer example

This example demonstrates the usage of software timers.
LED1 toggles in a timer callback function.

By default, LED1 is defined to pin 2. You could redefine it:

```sh
$ make clean build flash CFLAGS_EXTRA=-DLED1=3
```
