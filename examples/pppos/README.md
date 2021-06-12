# Networking over PPP

On linux/macos side, start slipterm with PPP mode
(change `/dev/cu.usbserial-1410` to your serial port):

```sh
$ ./tools/slipterm -p /dev/cu.usbserial-1410 -t -v
Opened master PTY, fd=3. Slave: /dev/ttys002
Opened /dev/cu.usbserial-1410 @ 115200 fd=4
```


Notice the slave PTY name. Start a PPP daemon, pass slave PTY name:

```sh
$ sudo pppd /dev/ttys002 115200 debug local proxyarp passive updetach noccp nocrtscts noauth 10.0.0.1:10.0.0.2
```

