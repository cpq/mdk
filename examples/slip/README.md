# Networking over SLIP

SLIP is a super simplistic way of injecting IP packets into a serial stream.
SLIP wraps a network packet in-between special marker characters,
per https://datatracker.ietf.org/doc/html/rfc1055

A device initialises an IP stack, CNIP, using SLIP on a hardware layer.
It reads UART, and if it detects a network frame, it pushes that network
frame to the CNIP stack for processing. Likewise, and outgoing network
frames get written to the UART.

On a workstation side, we run a terminal console with SLIP support.
A tool opens a raw socket. Then it reads the UART,
detects IP packets in UART output, and for each frame,
- writes that frame to the raw socket
- optionally, hexdumps that frame for debugging purposes
- from a received frame, a tool learns device's MAC address

When a tool receives frames on a raw socket, those frames with device's
destinaton MAC address, get written to the UART.

Each network frame includes all layers including MAC layer.
