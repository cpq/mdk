// Copyright (c) 2022 Cesanta
// All rights reserved

#pragma once

#define W5500_SOCK_MACRAW 0x42
#define W5500_SOCK_REG_CR_OPEN 1
#define W5500_SOCK_REG_MR_MACRAW 4
#define W5500_MR_RST 0x80
#define W5500_SOCK_REG_SR 3
#define W5500_SOCK_REG_CR 1
#define W5500_BLOCK_CR (0 << 3)
#define W5500_BLOCK_SR (1 << 3)
#define W5500_BLOCK_TX_BUF (2 << 3)
#define W5500_BLOCK_RX_BUF (3 << 3)
#define W5500_MODE_READ (0 << 2)
#define W5500_MODE_WRITE (1 << 2)
#define W5500_MR 0
#define W5500_SOCK_REG_MR 0
#define W5500_SHAR 0x9
#define W5500_SOCK_REG_RXBUF_SIZE 0x1e
#define W5500_SOCK_REG_TXBUF_SIZE 0x1f
#define W5500_IR_SENDOK 0x10
#define W5500_CR_RECV 0x40
#define W5500_IR_TIMEOUT 8
#define W5500_SOCK_CLOSED 0
#define W5500_CR_SEND 0x20
#define W5500_IR 2
#define W5500_TX_FSR 0x20
#define W5500_RX_RSR 0x26
#define W5500_RX_RD 0x28
#define W5500_TX_WR 0x24

struct w5500 {
  uint8_t mac[6];                           // MAC address
  void *spi;                                // Opaque SPI bus descriptor
  int cs;                                   // SPI chip select pin
  uint8_t (*txn)(struct w5500 *, uint8_t);  // SPI transaction
  void (*begin)(struct w5500 *);            // SPI begin
  void (*end)(struct w5500 *);              // SPI end
};

static inline void w5500_write(struct w5500 *w, uint8_t block, uint16_t address,
                               uint8_t wb) {
  spi_begin(w->spi, w->cs);
  block |= W5500_MODE_WRITE;
  spi_txn(w->spi, (uint8_t) ((address & 0xff00) >> 8));
  spi_txn(w->spi, (uint8_t) ((address & 0x00ff) >> 0));
  spi_txn(w->spi, block);
  spi_txn(w->spi, wb);
  spi_end(w->spi, w->cs);
}

static inline uint8_t w5500_read(struct w5500 *w, uint8_t block,
                                 uint16_t address) {
  uint8_t ret;
  spi_begin(w->spi, w->cs);
  block |= W5500_MODE_READ;
  spi_txn(w->spi, (uint8_t) ((address & 0xFF00) >> 8));
  spi_txn(w->spi, (uint8_t) ((address & 0x00FF) >> 0));
  spi_txn(w->spi, block);
  ret = spi_txn(w->spi, 0);
  spi_end(w->spi, w->cs);
  return ret;
}

static inline void w5500_write_buf(struct w5500 *w, uint8_t block,
                                   uint16_t address, const uint8_t *pBuf,
                                   uint16_t len) {
  uint16_t i;
  spi_begin(w->spi, w->cs);
  block |= W5500_MODE_WRITE;
  spi_txn(w->spi, (uint8_t) ((address & 0xFF00) >> 8));
  spi_txn(w->spi, (uint8_t) ((address & 0x00FF) >> 0));
  spi_txn(w->spi, block);
  for (i = 0; i < len; i++) spi_txn(w->spi, pBuf[i]);
  spi_end(w->spi, w->cs);
}

static inline void w5500_set_CR(struct w5500 *w, uint8_t cr) {
  w5500_write(w, W5500_BLOCK_SR, W5500_SOCK_REG_CR, cr);
  while (w5500_read(w, W5500_BLOCK_SR, W5500_SOCK_REG_CR)) (void) 0;
}

static inline uint16_t w5500_read_word(struct w5500 *w, uint8_t block,
                                       uint16_t address) {
  return ((uint16_t) (w5500_read(w, block, address) << 8) +
          w5500_read(w, block, address + 1));
}

static inline uint16_t w5500_get_TX_FSR(struct w5500 *w) {
  uint16_t val = 0, val1 = 0;
  do {
    val1 = w5500_read_word(w, W5500_BLOCK_SR, W5500_TX_FSR);
    if (val1 != 0) {
      val = w5500_read_word(w, W5500_BLOCK_SR, W5500_TX_FSR);
    }
  } while (val != val1);
  return val;
}

static inline uint16_t w5500_get_RX_RSR(struct w5500 *w) {
  uint16_t val = 0, val1 = 0;
  do {
    val1 = w5500_read_word(w, W5500_BLOCK_SR, W5500_RX_RSR);
    if (val1 != 0) {
      val = w5500_read_word(w, W5500_BLOCK_SR, W5500_RX_RSR);
    }
  } while (val != val1);
  return val;
}

static inline void w5500_write_word(struct w5500 *w, uint8_t block,
                                    uint16_t address, uint16_t word) {
  w5500_write(w, block, address, (uint8_t) (word >> 8));
  w5500_write(w, block, address + 1, (uint8_t) word);
}

static inline void w5500_recv_ignore(struct w5500 *w, uint16_t len) {
  uint16_t ptr;
  ptr = w5500_read_word(w, W5500_BLOCK_SR, W5500_RX_RD);
  ptr += len;
  w5500_write_word(w, W5500_BLOCK_SR, W5500_RX_RD, ptr);
}

static inline void w5500_read_buf(struct w5500 *w, uint8_t block,
                                  uint16_t address, uint8_t *pBuf,
                                  uint16_t len) {
  uint16_t i;
  w->begin(w);
  block |= W5500_MODE_READ;
  spi_txn(w->spi, (uint8_t) ((address & 0Xff00) >> 8));
  spi_txn(w->spi, (uint8_t) ((address & 0x00ff) >> 0));
  spi_txn(w->spi, block);
  for (i = 0; i < len; i++) pBuf[i] = spi_txn(w->spi, 0);
  w->end(w);
}

static inline void w5500_recv_data(struct w5500 *w, uint8_t *buf,
                                   uint16_t len) {
  uint16_t ptr;
  if (len == 0) return;
  ptr = w5500_read_word(w, W5500_BLOCK_SR, W5500_RX_RD);
  w5500_read_buf(w, W5500_BLOCK_RX_BUF, ptr, buf, len);
  ptr += len;
  w5500_write_word(w, W5500_BLOCK_SR, W5500_RX_RD, ptr);
}

static inline void w5500_send_data(struct w5500 *w, const uint8_t *buf,
                                   uint16_t len) {
  uint16_t ptr = 0;
  if (len == 0) return;
  ptr = w5500_read_word(w, W5500_BLOCK_SR, W5500_TX_WR);
  w5500_write_buf(w, W5500_BLOCK_TX_BUF, ptr, buf, len);
  ptr += len;
  w5500_write_word(w, W5500_BLOCK_SR, W5500_TX_WR, ptr);
}

static inline uint16_t w5500_rx(struct w5500 *w, uint8_t *buffer, uint16_t bs) {
  uint16_t len = w5500_get_RX_RSR(w);
  if (len > 0) {
    uint8_t head[2];
    uint16_t data_len = 0;
    w5500_recv_data(w, head, 2);
    w5500_set_CR(w, W5500_CR_RECV);
    data_len = head[0];
    data_len = (uint16_t) ((data_len << 8) + head[1]);
    data_len -= 2;
    if (data_len > bs) {
      w5500_recv_ignore(w, data_len);
      w5500_set_CR(w, W5500_CR_RECV);
      return 0;
    }
    w5500_recv_data(w, buffer, data_len);
    w5500_set_CR(w, W5500_CR_RECV);
    if ((buffer[0] & 0x01) || memcmp(&buffer[0], w->mac, 6) == 0) {
      return data_len;
    } else {
      return 0;
    }
  }
  return 0;
}

static inline int w5500_tx(struct w5500 *w, void *buf, uint16_t len) {
  while (1) {
    uint16_t freesize = w5500_get_TX_FSR(w);
    if (w5500_read(w, W5500_BLOCK_SR, W5500_SOCK_REG_SR) == W5500_SOCK_CLOSED)
      return 0;
    if (len <= freesize) break;
  };
  w5500_send_data(w, buf, len);
  w5500_set_CR(w, W5500_CR_SEND);
  while (1) {
    uint8_t tmp = w5500_read(w, W5500_BLOCK_SR, W5500_IR) & 0x1f;
    if (tmp & W5500_IR_SENDOK) {
      w5500_write(w, W5500_BLOCK_SR, W5500_IR, (W5500_IR_SENDOK & 0x1F));
      break;
    } else if (tmp & W5500_IR_TIMEOUT) {
      w5500_write(w, W5500_BLOCK_SR, W5500_IR, (W5500_IR_TIMEOUT & 0x1F));
      return 0;
    }
  }
  return len;
}

static inline int w5500_init(struct w5500 *w) {
  w->end(w);
  w5500_write(w, W5500_BLOCK_CR, W5500_MR, W5500_MR_RST);
  w5500_read(w, W5500_BLOCK_CR, W5500_MR);
  w5500_write(w, W5500_BLOCK_SR, W5500_SOCK_REG_RXBUF_SIZE, 16);
  w5500_write(w, W5500_BLOCK_SR, W5500_SOCK_REG_TXBUF_SIZE, 16);
  w5500_write_buf(w, W5500_BLOCK_CR, W5500_SHAR, w->mac, 6);
  w5500_write(w, W5500_BLOCK_SR, W5500_SOCK_REG_MR, W5500_SOCK_REG_MR_MACRAW);
  w5500_set_CR(w, W5500_SOCK_REG_CR_OPEN);
  return w5500_read(w, W5500_BLOCK_SR, W5500_SOCK_REG_SR) == W5500_SOCK_MACRAW;
}
