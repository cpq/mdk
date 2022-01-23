// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static inline unsigned long time_us(void) {
  return (unsigned long) time(NULL) * 1000000;
}

static inline void delay_ms(volatile unsigned long ms) {
  usleep((useconds_t) ms * 1000);
}

static inline void ledc_enable(void) {
}

static inline void wdt_disable(void) {
}

static inline void soc_init(void) {
}
