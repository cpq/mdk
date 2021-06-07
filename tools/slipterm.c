// Copyright (c) 2021 Charles Lohr
// All rights reserved

#define SLIPTERM_VERSION "0.01"

// GENERAL NOTEs/TODO:
//   1) Should we use ctrl+] like IDF?
//   2) Actually test.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define GENERAL_BUFFER_LENGTH 2048
#define NETWORK_BUFFER_LENGTH 2048


// This is from RFC1055 - BUT we have a departure.  SLIP_END puts the system
// into regular serial mode.  In order to enter packet mode, we expect to see
// SLIP_ESC followed by SLIP_END. SLIP_END has no effect in serial mode.
//
// It goes: 
//   STANDARD MODE (default) -> SLIP_ESC -> SLIP_END -> NETWORK MODE
//   NETWORK MODE -> SLIP_END -> STANDARD MODE
//   NETWORK MODE -> SLIP_ESC -> SLIP_END -> NETWORK MODE
//
// Therefore, you can't just resend a SLIP_END when in network mode to send
//  a new packet.

#define SLIP_END             0300    /* indicates end of packet */
#define SLIP_ESC             0333    /* indicates byte stuffing */
#define SLIP_ESC_END         0334    /* ESC ESC_END means END data byte */
#define SLIP_ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte */

#if 0
int iSerialPort;
int iRawSocket;
pthread_t thdNetworkReceive;
pthread_t thdUserInput;
struct termios tSavedTermios;
struct ifreq ifrInterface;
struct sockaddr_ll saRaw;

void *NetworkReceiveThread(void *v) {
  uint8_t rxbuff[NETWORK_BUFFER_LENGTH + 3] = {0};
  rxbuff[0] = SLIP_ESC;
  rxbuff[1] = SLIP_END;
  while (1) {
    int rx = recv(iRawSocket, rxbuff + 2, sizeof(rxbuff), 0);
    if (rx < 0) break;
    rxbuff[rx + 2] = SLIP_END;
    if (write(iSerialPort, rxbuff, rx + 3) < 0) {
      fprintf(stderr,
              "Warning: could not write network frame to SLIP socket\n");
    }
  }

  fprintf(stderr, "Error: Network disconnected. Exiting.\n");

  exit(-1);
}

void *UserInputThread(void *v) {
  // TODO: Configure user input to disable local echo.

  struct termios tp;
  if (tcgetattr(STDIN_FILENO, &tp) == -1) {
    fprintf(stderr, "Warning: Could not get local echo on user input.\n");
  }

  // Backup reg.
  tSavedTermios = tp;

  tp.c_lflag &= ~ECHO;    // Disable local echo.
  tp.c_lflag &= ~ICANON;  // Also disable canonical mode.

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) == -1) {
    fprintf(stderr, "Warning: Could not disable local echo on user input.\n");
  }

  int c;
  while ((c = getchar()) != EOF) {
    char ch[1] = {c};
    write(iSerialPort, ch, 1);
  }
}

void TXMessage(uint8_t *netbuf, int networkbufferplace) {
  // Acutally received a packet!
  saRaw.sll_ifindex = ifrInterface.ifr_ifindex;
  saRaw.sll_halen = ETH_ALEN;
  memcpy(saRaw.sll_addr, netbuf + 6, 6);

  if (sendto(iRawSocket, netbuf, networkbufferplace, 0,
             (struct sockaddr *) &socket_address,
             sizeof(struct sockaddr_ll)) < 0) {
    fprintf(stderr, "Warning: Could not transmit packet\n");
  }
}
#endif

static int s_signo;
void SignalHandler(int signo) {
  s_signo = signo;

#if 0
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tSavedTermios) == -1) {
    fprintf(stderr, "Warning: Could not re-enable local echo on user input.\n");
  }

  close(iSerialPort);
  close(iRawSocket);
#endif
}

int main(int argc, char **argv) {
  const char *baud = "115200";
  const char *port = NULL;
  const char *iface = NULL;
  const char *autodetect = NULL;

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      baud = argv[++i];
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      iface = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      port = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0) {
      autodetect = "yes";
    } else {
      fprintf(stderr, "slipterm version " SLIPTERM_VERSION
                      " usage:\n"
                      "\t-h print this help message\n"
                      "\t-r print autodetected port, and network and exit\n"
                      "\t-q don't print extra messages message start\n"
                      "\t-i [network interface to tap]\n"
                      "\t-p [port]\n"
                      "\t-b [baud rate to use on port]\n");
      exit(0);
    }
  }

#if 0
  // Tricky: If the baud rate given was bad, abort.
  if (selected_baud == 0) {
    fprintf(stderr, "Error: invalid baud rate.\n");
    return -4;
  }

  if (!port) {
    // No port selected. Autodetect.
    // Prefer ttyUSB to ttyACM.
    const char *searchports[] = {"/dev/ttyACM0", "/dev/ttyUSB0", 0};
    const char *sp;
    for (sp = searchports[0]; *sp; sp++) {
      struct stat statbuf = {0};
      if (stat(sp, &statbuf) == 0 && !S_ISDIR(statbuf)) {
        port = sp;
        break;
      }
    }
    if (!port) {
      fprintf(stderr, "Error: Could not select a serial port for operation.\n");
    }
  }

  if (!selected_iface) {
    // The following gist points out how much easier using /proc/net/route is
    // we are going to prefer that over `NETLINK_ROUTE`
    // https://gist.github.com/javiermon/6272065

    FILE *f = fopen("/proc/net/route", "r");
    if (f) {
      char iface[IF_NAMESIZE];
      char buf[GENERAL_BUFFER_LENGTH];
      while (fgets(buf, sizeof(buf), f)) {
        unsigned long dest;
        int r = sscanf(buf, "%s %lx", iface, &dest);
        if (r == 2 && dest == 0) {
          // Default gateway.
          selected_iface = strdup(iface);
          break;
        }
      }
      fclose(f);
    } else {
      fprintf(stderr,
              "Error: /proc/net/route unopenable. Need interface selected.\n");
      exit(-9);
    }

    if (!selected_iface) {
      fprintf(stderr, "Error: could not autodetect an interface.\n");
      exit(-9);
    }
  }

  if (printdetect) {
    fprintf(stderr,
            "slipterm version " SLIPTERM_VERSION
            " configuration:\ninterface %s\nport %s\nbaud%d\n",
            selected_iface, port, selected_baud);

    // If -r flag used, exit.
    if (printdetect == 2) exit(0);
  }

  // After this point, we are fully configured.
  iSerialPort = open(port, O_RDWR | O_SYNC);

  if (!iSerialPort) {
    fprintf(stderr, "Error: Could not open serial port \"%s\"\n",
            port);
    return -12;
  }

  // Set baud rates
  {
    struct termios2 tio;
    int r1 = ioctl(iSerialPort, TCGETS2, &tio);
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = selected_baud;
    tio.c_ospeed = selected_baud;
    int r2 = ioctl(iSerialPort, TCSETS2, &tio);
    if (r1 || r2) {
      fprintf(stderr, "Warning: Could not set baud %d on serial port \"%s\".\n",
              selected_baud, port);
    }
  }

  iRawSocket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (iRawSocket < 0) {
    fprintf(stderr, "Error: Can't open network device.\n");
    return -15;
  }

  {
    // Configure interface.
    int ifnamelen = strlen(selected_iface);
    if (ifnamelen >= sizeof(ifrInterface.ifr_name)) {
      fprintf(stderr, "Error: name of interface too long.\n");
      return -16;
    }
    memcpy(ifrInterface.ifr_name, selected_iface, ifnamelen);
    ifrInterface.ifr_name[ifnamelen] = 0;

    ioctl(iRawSocket, SIOCGIFINDEX, &ifrInterface);

    memset(&saRaw, 0, sizeof(saRaw));
    saRaw.sll_family = PF_PACKET;
    saRaw.sll_protocol = 0x0000;
    saRaw.sll_ifindex = ifrInterface.ifr_ifindex;
    saRaw.sll_hatype = 0;
    saRaw.sll_pkttype = PACKET_HOST;

    // Not sure why, this is required for receiving on some systems.
    int r = bind(iRawSocket, (const struct sockaddr *) &saRaw, sizeof(saRaw));
    if (r < 0) {
      fprintf(stderr, "Error: Could not bind to inerface. Error %d\n", errno);
      return -18;
    }

    r = setsockopt(iRawSocket, SOL_SOCKET, SO_BINDTODEVICE, selected_iface,
                   ifnamelen);

    if (r < 0) {
      fprintf(stderr,
              "Warning: Interface could not bind."
              "Networking probably will not work.\n");
    }
  }

  pthread_create(&thdNetworkReceive, 0, NetworkReceiveThread, 0);
  pthread_create(&thdUserInput, 0, UserInputThread, 0);


  // The main loop:
  //  Receive characters from the serial port and federate them appropriately.
  {
    enum {
      STATE_DEFAULT,
      STATE_DEFAULT_ESC,
      STATE_IN_PACKET,
      STATE_IN_PACKET_ESC,
    };
    int state = STATE_DEFAULT;

    uint8_t buf[GENERAL_BUFFER_LENGTH];
    uint8_t netbuf[NETWORK_BUFFER_LENGTH];
    uint8_t textbuffer[GENERAL_BUFFER_LENGTH];
    int networkbufferplace = 0;
    int textbufferplace = 0;

    while (1) {
      int rx = read(iSerialPort, buf, sizeof(buf));

      if (rx < 0) {
        fprintf(stderr, "Error: Serial port read failed.\n");
        return -19;
      }

      int i;
      for (i = 0; i < rx; i++) {
        uint8_t c = buf[i];
        switch (state) {
          case STATE_DEFAULT:
            switch (c) {
              case SLIP_ESC:
                state = STATE_DEFAULT_ESC;
                break;
              default:
                // SLIP_END is not special in terminal mode.
                textbuffer[textbufferplace++] = c;
                break;
            }
            break;

          case STATE_DEFAULT_ESC:
            switch (c) {
              case SLIP_ESC_ESC:
                textbuffer[textbufferplace++] = SLIP_ESC;
                break;
              case SLIP_ESC_END:
                textbuffer[textbufferplace++] = SLIP_END;
                break;
              case SLIP_END:
                state = STATE_IN_PACKET;
                break;
              default:
                // Undefined behavior. Spurious esc?  Could use for
                // other OOB comms.
                break;
            }
            break;

          case STATE_IN_PACKET:
            switch (c) {
              case SLIP_ESC:
                state = STATE_IN_PACKET_ESC;
                break;
              case SLIP_END: {
                TXMessage(netbuf, networkbufferplace);
                networkbufferplace = 0;
                state = STATE_DEFAULT;
                break;
              }
              default:
                netbuf[networkbufferplace++] = c;
                if (networkbufferplace >= NETWORK_BUFFER_LENGTH) {
                  // Packet too long. Abort back to terminal mode.
                  // We were probably in termianl mode anyway.
                  state = STATE_DEFAULT;
                }
                break;
            }
            break;

          case STATE_IN_PACKET_ESC:
            switch (c) {
              case SLIP_ESC:
              default:
                // Neither of these are valid. Perhaps incorrect?
                state = STATE_DEFAULT;
                break;
              case SLIP_END:
                // We use this as another mechanism to indicate send.
                // only difference here is we stay in network mode.
                TXMessage(netbuf, networkbufferplace);
                networkbufferplace = 0;
                state = STATE_IN_PACKET;
                break;
              case SLIP_ESC_ESC:
              case SLIP_ESC_END:
                netbuf[networkbufferplace++] =
                    (c == SLIP_ESC_ESC) ? SLIP_ESC : SLIP_END;

                if (networkbufferplace >= NETWORK_BUFFER_LENGTH) {
                  // Packet too long. Abort back to terminal mode.
                  // We were probably in termianl mode anyway.
                  state = STATE_DEFAULT;
                }
                state = STATE_IN_PACKET;
            }
            break;
        }
      }

      if (textbufferplace) {
        write(0, textbuffer, textbufferplace);
      }
    }
  }
#endif

  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  while (s_signo == 0) {
    sleep(1);
  }
  printf("Exiting on signal %d\n", s_signo);

  return 0;
}
