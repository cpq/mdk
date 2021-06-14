// Copyright (c) 2021 Cesanta
// All rights reserved

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cell.h"

#define TIMEOUT_MS 1000

#if 1
#include "log.h"
#define DEBUG(fmt, ...) sdk_log(fmt, __VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

void cell_rx(struct cell *cell, uint8_t c) {
  cell->buf[cell->len++] = c;                  // Append to recv buffer
  if (cell->len >= cell->size) cell->len = 0;  // Silent flush
}

static void handle_response(struct cell *cell, char *buf, int len) {
  while (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
  DEBUG("response: [%s]\n", buf);
  if (memcmp(buf, "OK", 2) == 0) {
    cell->cmd++;
    cell->expire = 0;
  } else if (memcmp(buf, "ERROR", 5) == 0) {
    cell->state = CELL_START;
  }
}

static void cmd_exec(struct cell *cell, unsigned long now) {
  if (cell->cmds == NULL || cell->cmds[cell->cmd] == NULL) return;
  const char *cmd = cell->cmds[cell->cmd];
  if (cell->expire == 0) {
    cell->tx(cmd, (int) strlen(cmd));
    cell->expire = now + TIMEOUT_MS;  // Set the expire
    cell->len = 0;                    // Clear off receive buffer
  } else if (cell->expire < now) {
    DEBUG("Expired: [%s]\n", cmd);
    cell->state = CELL_START;
  } else {
    int discard = 0;
    for (int i = 0; i < cell->len; i++) {
      if (cell->buf[i] != '\n') continue;
      handle_response(cell, &cell->buf[discard], i - discard);
      discard = i + 1;
    }
    memmove(cell->buf, &cell->buf[discard], (size_t)(cell->len - discard));
    cell->len -= discard;
  }
}

void cell_poll(struct cell *cell, unsigned long now) {
  switch (cell->state) {
    case CELL_START:
      cell->tx("+++\r\n", 5);
      cell->expire = now + 1500;
      cell->state = CELL_WAIT;
      break;
    case CELL_WAIT:
      if (now < cell->expire) break;
      cell->expire = 0;
      cell->cmd = 0;
      cell->state = CELL_SETUP;
      break;
    case CELL_SETUP:
      cmd_exec(cell, now);
      if (cell->state != CELL_SETUP) break;
      if (cell->cmds[cell->cmd] == NULL) cell->state = CELL_PPP;
      break;
    case CELL_PPP:
      break;
  }
}
