// Copyright (c) 2021 Charles Lohr
// All rights reserved

#define SLIPTERM_VERSION "0.01"

// GENERAL NOTEs/TODO:
//   1) Should we use ctrl+] like IDF?
//   2) Actually test.

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
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

void SignalHandler() {
  // Re-enable

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tSavedTermios) == -1) {
    fprintf(stderr, "Warning: Could not re-enable local echo on user input.\n");
  }

  close(iSerialPort);
  close(iRawSocket);
}
#endif

int main(int argc, char **argv) {
  char *selected_port = 0;
  int selected_baud = 115200;
  char *selected_iface = 0;
  char printdetect = 1;

#if 0
  if (argc > 1) {
    char **argvadvance = argv;
    char printhelp = 0;

    for (argvadvance = argv; argvadvance < (argv + argc); argvadvance++) {
      char *av = *argvadvance;
      char c;
      while (c = *(av++)) {
        char nextchar = *av;
        char extended = 0;
        switch (c) {
          // Long parameters
          case '-':
            if (nextchar != '-') break;

            // We have a long parameter, i.e. --port, --baud, --interface
            if (strcmp((av + 1), "port") == 0) extended = 'p';
            if (strcmp((av + 1), "baud") == 0) extended = 'b';
            if (strcmp((av + 1), "interface") == 0) extended = 'i';

            break;

          // Parameters which take an additional parameter.
          case 'i':
          case 'b':
          case 'p':
            if (nextchar) {
              printhelp = 1;
              goto exit_parse_early;
            } else {
              extended = c;
            }
            break;

          // Short parameters
          case 'r':
            printdetect = 2;
            break;
          case 'q':
            printdetect = 0;
            break;
          case 'h':
          default:
            printhelp = 1;
            goto exit_parse_early;
            break;
        }

        if (extended) {
          char *avnext =
              ((argvadvance + 1) < (argv + argc)) ? *(argvadvance + 1) : 0;

          switch (extended) {
            case 'i':
              selected_iface = avnext;
              break;
            case 'b':
              selected_baud = atoi(avnext);
              break;
            case 'p':
              selected_port = avnext;
              break;
          }
        }
      }
    }

  exit_parse_early:

    if (printhelp) {
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
#endif

  // Parse options
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
      selected_baud = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      selected_iface = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      selected_port = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0) {
      printdetect = 2;
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

  if (!selected_port) {
    // No port selected. Autodetect.
    // Prefer ttyUSB to ttyACM.
    const char *searchports[] = {"/dev/ttyACM0", "/dev/ttyUSB0", 0};
    const char *sp;
    for (sp = searchports[0]; *sp; sp++) {
      struct stat statbuf = {0};
      if (stat(sp, &statbuf) == 0 && !S_ISDIR(statbuf)) {
        selected_port = sp;
        break;
      }
    }
    if (!selected_port) {
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
            selected_iface, selected_port, selected_baud);

    // If -r flag used, exit.
    if (printdetect == 2) exit(0);
  }

  // After this point, we are fully configured.
  iSerialPort = open(selected_port, O_RDWR | O_SYNC);

  if (!iSerialPort) {
    fprintf(stderr, "Error: Could not open serial port \"%s\"\n",
            selected_port);
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
              selected_baud, selected_port);
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

  signal(SIGINT, SignalHandler);

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

  return 0;
}
