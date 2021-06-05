// Copyright (c) 2012-2021 Charles Lohr, MIT license
// All rights reserved

#pragma once

#include <stddef.h>
#include <stdint.h>

// Low level (hardware) API
struct cn_if {                        // CNIP network interface
  const char *name;                   // For debugging
  void (*out)(const void *, size_t);  // Frame sender function
  uint8_t mac[6];                     // MAC address
  uint32_t ip, mask, gw;              // Leave zeros to use DCHP
};
void cn_input(struct cn_if *, void *, size_t);  // Handle received frame
void cn_poll(struct cn_if *, unsigned long);    // Call periodically

// High level (user) API
// This is a user-defined callback function that gets called on various events
struct cn_conn;                                     // A connection - opaque
typedef void (*cn_fn_t)(struct cn_conn *, int ev);  // User function
enum {
  CN_EV_ERROR,    // Error! Use cn_get_error() to get the reason
  CN_EV_CONNECT,  // Client connection established
  CN_EV_ACCEPT,   // New server connection accepted
  CN_EV_READ,     // More data arrived, use cn_rx() to access data
  CN_EV_WRITE,    // Data has been sent to the network
  CN_EV_CLOSE,    // Connection is about to vanish
};
// API for creating new connections
struct cn_conn *cn_tcp_listen(const char *url, cn_fn_t handler_fn);
struct cn_conn *cn_tcp_connect(const char *url, cn_fn_t handler_fn);
// API for use from inside a user callback function
void cn_rx(struct cn_conn *, void **buf, size_t *len);   // Get recv data
void cn_tx(struct cn_conn *, void **buf, size_t *len);   // Get sent data
void cn_send(struct cn_conn *, void *buf, size_t len);   // Send more data
void cn_free(struct cn_conn *, size_t off, size_t len);  // Discard recv data
