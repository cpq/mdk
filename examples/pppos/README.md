# Networking over PPP

On linux/macos side, start a PPP daemon

```sh
$ pppd /dev/SERIAL_PORT 115200 debug local passive updetach noccp nocrtscts 10.0.0.1:10.0.0.2
```