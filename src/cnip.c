// Copyright (c) 2012-2021 Charles Lohr, MIT license
// All rights reserved

#include <cnip.h>

#if 1
#include <sdk.h>
#define DEBUG(fmt, ...) sdk_log(fmt, __VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

struct eth {
  uint8_t dest_mac[6];
  uint8_t from_mac[6];
  uint8_t hwtype;
  uint8_t pktype;
} __attribute__((packed));

struct ip {
  uint8_t ip_fmt;
  uint8_t differentiated_services;  // Not used.
  uint16_t total_len;
  uint16_t identification;  // unused
  uint16_t frag_offset_and_flags;
  uint8_t ttl;
  uint8_t proto;
  uint16_t checksum;
  uint32_t src_ip;
  uint32_t dest_ip;
} __attribute__((packed));

static void cn_arp(struct cn_if *ifp, struct eth *eth, void *buf, size_t len) {
  (void) ifp;
  (void) eth;
  (void) buf;
  (void) len;
}

void cn_input(struct cn_if *ifp, void *buf, size_t len) {
  DEBUG("got frame %u bytes", len);
  struct eth *eth = buf;
  if (len < sizeof(*eth)) return;  // Truncated packet - runt?
  if (eth->hwtype != 8) return;    // Not ethernet
  if (eth->pktype == 6) {
    cn_arp(ifp, eth, eth + 1, len - sizeof(*eth));
  } else {
    struct ip *ip = (struct ip *) (eth + 1);
    if (len < sizeof(*eth) + sizeof(*ip)) return;  // Truncated packed
    if (ip->ip_fmt != 0x45) return;                // Not IP
  }
  (void) ifp;
}

void cn_poll(struct cn_if *ifp, unsigned long now_ms) {
  (void) ifp;
  (void) now_ms;
}
