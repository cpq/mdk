#define GPIO_FUNC_OUT_SEL REG(0X3ff44530)
#define GPIO_OUT_REG REG(0X3ff44004)
#define GPIO_ENABLE_REG REG(0X3ff44020)

static inline void gpio_output(int pin) {
  GPIO_FUNC_OUT_SEL[pin] = 256;    // Configure GPIO as a simple output
  GPIO_ENABLE_REG[0] |= BIT(pin);  // Enable pin. See TRM 4.3.3
}

static inline void gpio_toggle(int pin) {
  GPIO_OUT_REG[0] ^= BIT(pin);
}
