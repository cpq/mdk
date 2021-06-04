static inline int uart_rx(unsigned char *c) {
  extern int uart_rx_one_char(unsigned char *);
  return uart_rx_one_char(c);
}

static inline int uart_tx(unsigned char byte) {
  extern int uart_tx_one_char(unsigned char);
  return uart_tx_one_char(byte);
}
