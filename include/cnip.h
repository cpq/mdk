// Copyright (c) 2012-2021 Charles Lohr, MIT license
// All rights reserved

#pragma once

#include <stddef.h>

// Low level (hardware) API
void cn_mac_in(void *buf, size_t len);   // Implemented by CNIP
void cn_mac_out(void *buf, size_t len);  // Must be implemented by user
void cn_poll(unsigned long now_ms);      // Called periodically or on event

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
