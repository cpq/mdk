# Button example

Switch on an LED based on a button status.
This example does not implement any debouncing.

```sh
$ make clean build flash CFLAGS_EXTRA="-DLED1=1 -DBUTTON1=9"
```
