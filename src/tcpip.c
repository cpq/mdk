// Copyright (c) 2021 Cesanta
// All rights reserved

#include "tcpip.h"
#include <string.h>

struct lcp {
  uint8_t addr, ctrl, proto[2], code, id, len[2];
} __attribute__((packed));

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

#define U16(ptr) ((((uint16_t)(ptr)[0]) << 8) | (ptr)[1])
#define NET16(x) __builtin_bswap16(x)
#define NET32(x) __builtin_bswap32(x)

#define CNIP_ARP_CACHE_SIZE 10
static struct arp_entry s_arp_cache[CNIP_ARP_CACHE_SIZE];  // ARP cache
static size_t s_arp_idx;                                   // Current ARP index

static void net_arp(struct net_if *ifp, struct eth *eth, struct arp *arp) {
  if (arp->op == NET16(1) && arp->tpa == ifp->ip) {
    // ARP request. Edit packet in-place. Make a response, then send
    memcpy(eth->dst, eth->src, sizeof(eth->dst));
    memcpy(eth->src, ifp->mac, sizeof(eth->src));
    arp->op = NET16(2);
    memcpy(arp->tha, arp->sha, sizeof(arp->tha));
    memcpy(arp->sha, ifp->mac, sizeof(arp->sha));
    arp->tpa = arp->spa;
    arp->spa = ifp->ip;
    ifp->dbg("%s", "ARP response\n");
    ifp->out(eth, sizeof(*eth) + sizeof(*arp));
  } else if (arp->op == NET16(2)) {
    // ARP response
    if (memcmp(arp->tha, ifp->mac, sizeof(arp->tha)) != 0) return;
    // s_arp_cache[s_arp_idx++] = *(struct arp_entry *) (void *) &arp->sha;
    (void) s_arp_cache;
    if (s_arp_idx >= CNIP_ARP_CACHE_SIZE) s_arp_idx = 0;
  }
}

static uint16_t ipcsum(const uint8_t *p, const uint8_t *end) {
  uint32_t sum = 0;
  while (p < end) sum += (unsigned) ((((uint16_t) p[1]) << 8) | p[0]), p += 2;
  while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
  return ~sum & 0xffff;
}

static void net_icmp(struct net_if *ifp, struct eth *eth, struct ip *ip,
                     struct icmp *icmp, size_t len) {
  if (icmp->type == 8) {
    memcpy(eth->dst, eth->src, sizeof(eth->dst));
    memcpy(eth->src, ifp->mac, sizeof(eth->src));
    ip->dst = ip->src;
    ip->src = ifp->ip;
    ip->csum = 0;  // Important - clear csum before recomputing
    ip->csum = ipcsum((uint8_t *) ip, (uint8_t *) (ip + 1));
    icmp->type = 0;
    icmp->csum = 0;  // Important - clear csum before recomputing
    icmp->csum = ipcsum((uint8_t *) icmp, (uint8_t *) (icmp + 1) + len);
    size_t n = (size_t)((char *) (icmp + 1) - (char *) eth) + len;
    ifp->dbg("ICMP response %d\n", n);
    ifp->out(eth, n);
  }
}

static void net_ip(struct net_if *ifp, struct eth *eth, struct ip *ip,
                   size_t len) {
  if (ip->proto == 1) {
    struct icmp *icmp = (struct icmp *) (ip + 1);
    if (len < sizeof(*icmp)) return;
    ifp->dbg("ICMP %d\n", len);
    net_icmp(ifp, eth, ip, icmp, len - sizeof(*icmp));
  }
}

void net_input(struct net_if *ifp, void *buf, size_t mtu, size_t len) {
  // ifp->dbg("got frame %u bytes\n", len);
  struct eth *eth = buf;
  if (len < sizeof(*eth)) return;  // Truncated packet - runt?
  if (eth->type == NET16(0x806)) {
    struct arp *arp = (struct arp *) (eth + 1);
    if (sizeof(*eth) + sizeof(*arp) > len) return;  // Truncated
    ifp->dbg("ARP %d\n", len);
    net_arp(ifp, eth, arp);
  } else if (eth->type == NET16(0x800)) {
    struct ip *ip = (struct ip *) (eth + 1);
    if (len < sizeof(*eth) + sizeof(*ip)) return;  // Truncated packed
    if (ip->ver != 0x45) return;                   // Not IP
    ifp->dbg("IP %d\n", len);
    net_ip(ifp, eth, ip, len - sizeof(*eth) - sizeof(*ip));
  }
  (void) mtu;
}

static size_t ppp_decode(uint8_t *src, size_t len, uint8_t *dst) {
  size_t n = 0;
  for (size_t i = 0; i < len; i++) {
    if (src[i] != 0x7d) dst[n++] = src[i];
    if (src[i] == 0x7d && i + 1 < len) dst[n++] = src[++i] ^ 0x20;
  }
  return n;
}

static uint16_t mkfcs(const uint8_t *buf, size_t len) {
  uint16_t v = 0xffff;
  for (size_t i = 0; i < len; i++) {
    uint8_t c = buf[i];
    for (int j = 0; j < 8; j++) {
      v = ((v ^ c) & 1) ? (v >> 1) ^ 0x8408 : v >> 1, c >>= 1;
    }
  }
  return ~v;
}

static size_t ppp_encode(uint8_t *src, size_t len, uint8_t *dst) {
  size_t n = 0;
  dst[n++] = 0x7e;
  for (size_t i = 0; i < len; i++) {
    if (src[i] < 0x20 || src[i] == 0x7d || src[i] == 0x7e) {
      dst[n++] = 0x7d, dst[n++] = src[i] ^ 0x20;
    } else {
      dst[n++] = src[i];
    }
  }
  uint16_t fcs = mkfcs(src, len);
  dst[n++] = fcs & 255U;
  dst[n++] = (fcs >> 8) & 255U;
  dst[n++] = 0x7e;
  return n;
}

size_t ppp_input(struct net_if *ifp, void *buf, size_t len) {
  // Scan input, find two 0x7e begin/end markers.
  uint8_t *p = buf, cnt = 0;
  for (size_t n = 0; n < len; n++) {
    if (p[n] == 0x7e) cnt++;
    if (cnt == 2) {
      if (n < 4) return n + 1;
      // ifp->dbg("PPP frame: %d bytes\n", n + 1);
      // sdk_hexdump(buf, n + 1);
      uint8_t tmp[n + 2];
      size_t dec = ppp_decode(p + 1, n - 3, tmp);
      ifp->dbg("PPP decoded: %d bytes\n", dec);
      // sdk_hexdump(tmp, dec);

      struct lcp *lcp = (struct lcp *) tmp;
      // ifp->dbg("LCP %d %d %d\n", dec, sizeof(*lcp), (int) U16(lcp->len));
      if (dec >= sizeof(*lcp) && lcp->addr == 0xff && lcp->ctrl == 0x3 &&
          lcp->proto[0] == 0xc0 && lcp->proto[1] == 0x21) {
        if (lcp->code == 1) {        // Config req
          if (U16(lcp->len) == 4) {  // We've got "no options" req
            lcp->code = 2;           // Ack
            ifp->dbg("ACK\n");
            // sdk_hexdump(tmp, dec);
            size_t enc = ppp_encode(tmp, dec, buf);
            ifp->out(buf, enc);
            lcp->code = 1;  // request no options
            enc = ppp_encode(tmp, dec, buf);
            ifp->out(buf, enc);
          } else {
            lcp->code = 4;  // Reject. Ask for "no options"
            size_t enc = ppp_encode(tmp, dec, buf);
            ifp->dbg("REJ\n");
            // sdk_hexdump(tmp, dec);
            ifp->out(buf, enc);
          }
        } else if (lcp->code == 5) {
          // LCP end
        }
      }
      return n + 1;
    }
  }
  return 0;
}

void net_poll(struct net_if *ifp, unsigned long now_ms) {
  (void) ifp;
  (void) now_ms;
}
