// Copyright (c) 2022 Cesanta
// All rights reserved

#pragma once

#define WIZ_SOCK_MACRAW 0x42
#define WIZ_SOCK_REG_CR_OPEN 1
#define WIZ_SOCK_REG_MR_MACRAW 4
#define WIZ_MR_RST 0x80
#define WIZ_SOCK_REG_SR 3
#define WIZ_SOCK_REG_CR 1
#define WIZ_BLOCK_CR (0 << 3)
#define WIZ_BLOCK_SR (1 << 3)
#define WIZ_BLOCK_TX_BUF (2 << 3)
#define WIZ_BLOCK_RX_BUF (3 << 3)
#define WIZ_MODE_READ (0 << 2)
#define WIZ_MODE_WRITE (1 << 2)
#define WIZ_MR 0
#define WIZ_SOCK_REG_MR 0
#define WIZ_SHAR 0x9
#define WIZ_SOCK_REG_RXBUF_SIZE 0x1e
#define WIZ_SOCK_REG_TXBUF_SIZE 0x1f
#define WIZ_IR_SENDOK 0x10
#define WIZ_CR_RECV 0x40
#define WIZ_IR_TIMEOUT 8
#define WIZ_SOCK_CLOSED 0
#define WIZ_CR_SEND 0x20
#define WIZ_IR 2
#define WIZ_TX_FSR 0x20
#define WIZ_RX_RSR 0x26
#define WIZ_RX_RD 0x28
#define WIZ_TX_WR 0x24

struct w5500 {
  uint8_t mac[6];                           // MAC address
  int miso, mosi, clk, cs;                  // SPI pins
  uint8_t (*txn)(struct w5500 *, uint8_t);  // SPI transaction
  void (*begin)(struct w5500 *);            // SPI begin
  void (*end)(struct w5500 *);              // SPI end
};

static inline void wiz_write(struct w5500 *w, uint8_t block, uint16_t address,
                             uint8_t wb) {
  w->begin(w);
  block |= WIZ_MODE_WRITE;
  w->txn(w, (address & 0xFF00) >> 8);
  w->txn(w, (address & 0x00FF) >> 0);
  w->txn(w, block);
  w->txn(w, wb);
  w->end(w);
}

static inline uint8_t wiz_read(struct w5500 *w, uint8_t block,
                               uint16_t address) {
  uint8_t ret;
  w->begin(w);
  block |= WIZ_MODE_READ;
  w->txn(w, (address & 0xFF00) >> 8);
  w->txn(w, (address & 0x00FF) >> 0);
  w->txn(w, block);
  ret = w->txn(w, 0);
  w->end(w);
  return ret;
}

static inline void wiz_write_buf(struct w5500 *w, uint8_t block,
                                 uint16_t address, const uint8_t *pBuf,
                                 uint16_t len) {
  uint16_t i;
  w->begin(w);
  block |= WIZ_MODE_WRITE;
  w->txn(w, (address & 0xFF00) >> 8);
  w->txn(w, (address & 0x00FF) >> 0);
  w->txn(w, block);
  for (i = 0; i < len; i++) w->txn(w, pBuf[i]);
  w->end(w);
}

static inline void wiz_set_CR(struct w5500 *w, uint8_t cr) {
  wiz_write(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_CR, cr);
  while (wiz_read(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_CR)) (void) 0;
}

static inline uint16_t wiz_read_word(struct w5500 *w, uint8_t block,
                                     uint16_t address) {
  return ((uint16_t) wiz_read(w, block, address) << 8) +
         wiz_read(w, block, address + 1);
}

static inline uint16_t wiz_get_TX_FSR(struct w5500 *w) {
  uint16_t val = 0, val1 = 0;
  do {
    val1 = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_TX_FSR);
    if (val1 != 0) {
      val = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_TX_FSR);
    }
  } while (val != val1);
  return val;
}

static inline uint16_t wiz_get_RX_RSR(struct w5500 *w) {
  uint16_t val = 0, val1 = 0;
  do {
    val1 = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_RX_RSR);
    if (val1 != 0) {
      val = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_RX_RSR);
    }
  } while (val != val1);
  return val;
}

static inline void wiz_write_word(struct w5500 *w, uint8_t block,
                                  uint16_t address, uint16_t word) {
  wiz_write(w, block, address, (uint8_t) (word >> 8));
  wiz_write(w, block, address + 1, (uint8_t) word);
}

static inline void wiz_recv_ignore(struct w5500 *w, uint16_t len) {
  uint16_t ptr;
  ptr = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_RX_RD);
  ptr += len;
  wiz_write_word(w, WIZ_BLOCK_SR, WIZ_RX_RD, ptr);
}

static inline void wiz_read_buf(struct w5500 *w, uint8_t block,
                                uint16_t address, uint8_t *pBuf, uint16_t len) {
  uint16_t i;
  w->begin(w);
  block |= WIZ_MODE_READ;
  w->txn(w, (address & 0xFF00) >> 8);
  w->txn(w, (address & 0x00FF) >> 0);
  w->txn(w, block);
  for (i = 0; i < len; i++) pBuf[i] = w->txn(w, 0);
  w->end(w);
}

static inline void wiz_recv_data(struct w5500 *w, uint8_t *buf, uint16_t len) {
  uint16_t ptr;
  if (len == 0) return;
  ptr = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_RX_RD);
  wiz_read_buf(w, WIZ_BLOCK_RX_BUF, ptr, buf, len);
  ptr += len;
  wiz_write_word(w, WIZ_BLOCK_SR, WIZ_RX_RD, ptr);
}

static inline void wiz_send_data(struct w5500 *w, const uint8_t *buf,
                                 uint16_t len) {
  uint16_t ptr = 0;
  if (len == 0) return;
  ptr = wiz_read_word(w, WIZ_BLOCK_SR, WIZ_TX_WR);
  wiz_write_buf(w, WIZ_BLOCK_TX_BUF, ptr, buf, len);
  ptr += len;
  wiz_write_word(w, WIZ_BLOCK_SR, WIZ_TX_WR, ptr);
}

static inline uint16_t w5500_rx(struct w5500 *w, uint8_t *buffer, uint16_t bs) {
  uint16_t len = wiz_get_RX_RSR(w);

  if (len > 0) {
    uint8_t head[2];
    uint16_t data_len = 0;

    wiz_recv_data(w, head, 2);
    wiz_set_CR(w, WIZ_CR_RECV);

    data_len = head[0];
    data_len = (data_len << 8) + head[1];
    data_len -= 2;

    if (data_len > bs) {
      // Packet is bigger than buffer - drop the packet
      wiz_recv_ignore(w, data_len);
      wiz_set_CR(w, WIZ_CR_RECV);
      return 0;
    }

    wiz_recv_data(w, buffer, data_len);
    wiz_set_CR(w, WIZ_CR_RECV);

    // Had problems with W5500 MAC address filtering (the WIZ_MR_MFEN option)
    // Do it in software instead:
    if ((buffer[0] & 0x01) || memcmp(&buffer[0], w->mac, 6) == 0) {
      // Addressed to an Ethernet multicast address or our unicast address
      return data_len;
    } else {
      return 0;
    }
  }
  return 0;
}

static inline uint16_t w5500_tx(struct w5500 *w, void *buf, uint16_t len) {
  while (1) {  // Wait for space in the transmit buffer
    uint16_t freesize = wiz_get_TX_FSR(w);
    if (wiz_read(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_SR) == WIZ_SOCK_CLOSED)
      return -1;
    if (len <= freesize) break;
  };
  wiz_send_data(w, buf, len);
  wiz_set_CR(w, WIZ_CR_SEND);
  while (1) {
    uint8_t tmp = wiz_read(w, WIZ_BLOCK_SR, WIZ_IR) & 0x1f;
    if (tmp & WIZ_IR_SENDOK) {
      wiz_write(w, WIZ_BLOCK_SR, WIZ_IR, (WIZ_IR_SENDOK & 0x1F));
      break;
    } else if (tmp & WIZ_IR_TIMEOUT) {
      wiz_write(w, WIZ_BLOCK_SR, WIZ_IR, (WIZ_IR_TIMEOUT & 0x1F));
      return -1;  // There was a timeout
    }
  }
  return len;
}

static inline int w5500_init(struct w5500 *w) {
  w->end(w);
  // SPI.begin();
  // SPI.setClockDivider(SPI_CLOCK_DIV4);  // 4 MHz?
  // SPI.setBitOrder(MSBFIRST);
  // SPI.setDataMode(SPI_MODE0);
  wiz_write(w, WIZ_BLOCK_CR, WIZ_MR, WIZ_MR_RST);
  wiz_read(w, WIZ_BLOCK_CR, WIZ_MR);
  wiz_write(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_RXBUF_SIZE, 16);
  wiz_write(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_TXBUF_SIZE, 16);
  wiz_write_buf(w, WIZ_BLOCK_CR, WIZ_SHAR, w->mac, 6);
  wiz_write(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_MR, WIZ_SOCK_REG_MR_MACRAW);
  wiz_set_CR(w, WIZ_SOCK_REG_CR_OPEN);
  return wiz_read(w, WIZ_BLOCK_SR, WIZ_SOCK_REG_SR) == WIZ_SOCK_MACRAW;
}
