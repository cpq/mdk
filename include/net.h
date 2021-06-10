// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stddef.h>
#include <stdint.h>

// Low level (hardware) API
struct net_if {
  void (*out)(const void *, size_t);  // Frame sender function
  uint8_t mac[6];                     // MAC address
  uint32_t ip, mask, gw;              // Leave zeros to use DCHP
};
void net_input(struct net_if *, void *, size_t, size_t);  // Handle frame
void net_poll(struct net_if *, unsigned long);            // Call periodically

// High level (user) API
struct net_conn;  // A network connection
typedef void (*net_fn_t)(struct net_conn *, int ev, void *ev_data);

#define NET_EV_ERROR 1    // Error            ev_data: const char *err_str
#define NET_EV_CONNECT 2  // Connected to peer         NULL
#define NET_EV_ACCEPT 3   // New connection accepted   NULL
#define NET_EV_READ 4     // Data received             void **[buf, len]
#define NET_EV_CLOSE 5    // Connection is closing     NULL
#define NET_EV_POLL 6     // Periodic poll             NULL

struct net_conn *net_listen_tcp(const char *url, net_fn_t fn);
struct net_conn *net_connect_tcp(const char *url, net_fn_t fn);
void net_send(struct net_conn *, void *buf, size_t len);
