#pragma once

struct cell {
  int state;                         // Current state, CELL_* enumeration
  unsigned char *buf;                // Received UART data buffer
  size_t size;                       // Received UART buffer size
  size_t len;                        // Received UART data len
  const char *cmds;                  // Setup commands, NULL terminated
  int cmd;                           // Current setup command
  unsigned long expire;              // Command expiration timestamp
  void (*tx)(const void *, size_t);  // Transmit function
  void (*dbg)(const char *, ...);    // Debug log function
};

enum { CELL_START, CELL_WAIT, CELL_AT, CELL_OK, CELL_PPP };  // Modem states

void cell_input(struct cell *, unsigned char);        // Feed a character
void cell_poll(struct cell *, unsigned long now_ms);  // Handle cell
