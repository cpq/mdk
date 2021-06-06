// Copyright (c) 2012-2021 Charles Lohr, MIT license
// All rights reserved

#pragma once

#include <stddef.h>
#include <stdint.h>

// Low level (hardware) API
struct cnip_if {
  void (*out)(const void *, size_t);  // Frame sender function
  uint8_t mac[6];                     // MAC address
  size_t mtu;                         // Max frame size
  uint32_t ip, mask, gw;              // Leave zeros to use DCHP
};
void cnip_input(struct cnip_if *, void *, size_t);  // Handle received frame
void cnip_poll(struct cnip_if *, unsigned long);    // Call periodically

// High level (user) API
struct cnip_conn;  // A network connection
typedef void (*cnip_fn_t)(struct cnip_conn *, int ev, void *ev_data);

#define CNIP_EV_ERROR 1    // Error            ev_data: const char *err_str
#define CNIP_EV_CONNECT 2  // Connected to peer         NULL
#define CNIP_EV_ACCEPT 3   // New connection accepted   NULL
#define CNIP_EV_READ 4     // Data received             void **[buf, len]
#define CNIP_EV_CLOSE 5    // Connection is closing     NULL
#define CNIP_EV_POLL 6     // Periodic poll             NULL

struct cnip_conn *cnip_listen_tcp(const char *url, cnip_fn_t fn);
struct cnip_conn *cnip_connect_tcp(const char *url, cnip_fn_t fn);
void cnip_send(struct cnip_conn *, void *buf, size_t len);
