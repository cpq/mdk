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
  uint8_t dst[6];  // Destination MAC address
  uint8_t src[6];  // Source MAC address
  uint16_t type;   // Ethernet type
} __attribute__((packed));

struct ip {
  uint8_t ver;    // Version
  uint8_t tos;    // Unused
  uint16_t len;   // Length
  uint16_t id;    // Unused
  uint16_t frag;  // Fragmentation
  uint8_t ttl;    // Time to live
  uint8_t proto;  // Upper level protocol
  uint16_t csum;  // Checksum
  uint32_t src;   // Source IP
  uint32_t dst;   // Destination IP
} __attribute__((packed));

struct icmp {
  uint8_t type;
  uint8_t code;
  uint16_t csum;
  uint16_t id;
} __attribute__((packed));

struct arp {
  uint16_t fmt;    // Format of hardware address
  uint16_t pro;    // Format of protocol address
  uint8_t hlen;    // Length of hardware address
  uint8_t plen;    // Length of protocol address
  uint16_t op;     // Operation
  uint8_t sha[6];  // Sender hardware address
  uint32_t spa;    // Sender protocol address
  uint8_t tha[6];  // Target hardware address
  uint32_t tpa;    // Target protocol address
} __attribute__((packed));

struct tcp {
  uint16_t sport;  // Source port
  uint16_t dport;  // Sestination port
  uint32_t seq;    // Sequence number
  uint32_t ack;    // Acknowledgement number
  uint8_t off;     // Data offset
  uint8_t flags;   // TCP flags
  uint16_t win;    // Window
  uint16_t csum;   // Checksum
  uint16_t surp;   // Urgent pointer
} __attribute__((packed));
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN | TH_SYN | TH_RST | TH_ACK | TH_URG | TH_ECE | TH_CWR)

struct udp {
  uint16_t sport;  // Source port
  uint16_t dport;  // Destination port
  uint16_t len;    // UDP length
  uint16_t csum;   // UDP checksum
} __attribute__((packed));

struct arp_entry {
  uint8_t mac[6];
  uint32_t ip;
} __attribute__((packed));

#define CNIP_ARP_CACHE_SIZE 10
static struct arp_entry s_arp_cache[CNIP_ARP_CACHE_SIZE];  // ARP cache
static size_t s_arp_idx;                                   // Current ARP index

static void cnip_arp(struct cnip_if *ifp, struct eth *eth, struct arp *arp) {
  if (arp->op == 1 && arp->tpa == ifp->ip) {
    // ARP request
    memcpy(eth->dst, eth->src, sizeof(eth->dst));
    memcpy(eth->src, ifp->mac, sizeof(eth->src));
    arp->op = 2;
    memcpy(arp->tha, arp->sha, sizeof(arp->tha));
    memcpy(arp->sha, ifp->mac, sizeof(arp->sha));
    arp->tpa = arp->spa;
    arp->spa = ifp->ip;
    ifp->out(eth, (size_t)((char *) (arp + 1) - (char *) eth));
  } else if (arp->op == 2) {
    // ARP response
    if (memcmp(arp->tha, ifp->mac, sizeof(arp->tha)) != 0) return;
    s_arp_cache[s_arp_idx++] = *(struct arp_entry *) &arp->sha;
    if (s_arp_idx >= CNIP_ARP_CACHE_SIZE) s_arp_idx = 0;
  }
}

static void cnip_icmp(struct cnip_if *ifp, struct eth *eth, struct ip *ip,
                      struct icmp *icmp, size_t len) {
  if (icmp->type == 8) {
    memcpy(eth->dst, eth->src, sizeof(eth->dst));
    memcpy(eth->src, ifp->mac, sizeof(eth->src));
    ip->dst = ip->src;
    ip->src = ifp->ip;
    icmp->type = 0;
    ifp->out(eth, (size_t)((char *) (icmp + 1) - (char *) eth) + len);
  }
}

static void cnip_ip(struct cnip_if *ifp, struct eth *eth, struct ip *ip,
                    size_t len) {
  if (ip->proto == 1) {
    struct icmp *icmp = (struct icmp *) (ip + 1);
    if (len < sizeof(*icmp)) return;
    cnip_icmp(ifp, eth, ip, icmp, len - sizeof(*icmp));
  }
}

void cnip_input(struct cnip_if *ifp, void *buf, size_t len) {
  DEBUG("got frame %u bytes", len);
  struct eth *eth = buf;
  if (len < sizeof(*eth)) return;  // Truncated packet - runt?
  if (eth->type == 0x806) {
    struct arp *arp = (struct arp *) (eth + 1);
    if (sizeof(*eth) + sizeof(*arp) > len) return;  // Truncated
    cnip_arp(ifp, eth, arp);
  } else if (eth->type == 0x800) {
    struct ip *ip = (struct ip *) (eth + 1);
    if (len < sizeof(*eth) + sizeof(*ip)) return;  // Truncated packed
    if (ip->ver != 0x45) return;                   // Not IP
    cnip_ip(ifp, eth, ip, len - sizeof(*eth) - sizeof(*ip));
  }
}

void cnip_poll(struct cnip_if *ifp, unsigned long now_ms) {
  (void) ifp;
  (void) now_ms;
}
