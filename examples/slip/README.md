# Networking over SLIP

SLIP is a super simplistic way of injecting IP packets into a serial stream.
SLIP wraps a network packet in-between special marker characters,
per https://datatracker.ietf.org/doc/html/rfc1055

A device initialises CNIP stack using SLIP driver.
It reads UART, and if it detects a network frame, it pushes that network
frame to the CNIP stack for processing. Outgoing frames get written to the UART.

On a workstation side, we run a terminal console with SLIP support.
A tool opens a raw socket. Then it reads the UART,
detects IP packets in UART output, and for each frame,
- writes that frame to the raw socket
- optionally, hexdumps that frame for debugging purposes
- from a received frame, learns device's MAC address

When a tool receives frames on a raw socket, those frames with device's
destinaton MAC address, get written to the UART.

Each network frame includes all layers including MAC layer.

# Usage

In one terminal, build and flash the firmware:

```sh
$ make clean build flash
```

In another terminal, start slipterm (change `/dev/cu.usbserial-1410`
to your serial port and `en0` to your network iface):

```sh
$ make -C ../../tools
$ ../../tools/slipterm -p /dev/cu.usbserial-1410 -i en0 -v -f "host 192.168.0.7 or ether host ff:ff:ff:ff:ff:ff"
Opened master PTY, fd=3. Slave: /dev/ttys002
Opened /dev/cu.usbserial-1410 @ 115200 fd=4
```
