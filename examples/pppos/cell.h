// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

// Hardware independent cellular modem API. Cellular modems follow GSM 3GPP
// specifications which standartises AT commands

struct cell {
  int state;                      // Current state, CELL_* enumeration
  char *buf;                      // Received UART data buffer
  int size;                       // Received UART buffer size
  int len;                        // Received UART data len
  const char **cmds;              // Setup commands, NULL terminated
  int cmd;                        // Current setup command
  unsigned long expire;           // Command expiration timestamp
  void (*tx)(const void *, int);  // Transmit function
};

enum { CELL_START, CELL_WAIT, CELL_SETUP, CELL_PPP };  // Modem states

void cell_input(struct cell *, unsigned char);        // Feed a character
void cell_poll(struct cell *, unsigned long now_ms);  // Handle cell
