// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BIT(x) (1UL << (x))
#define REG(x) ((volatile uint32_t *) (x))

#ifndef LED1
#define LED1 1  // Default LED pin
#endif

#ifndef BTN1
#define BTN1 9  // Default user button pin
#endif

#include "gpio.h"
#include "ledc.h"
#include "log.h"
#include "soc.h"
#include "spi.h"
#include "sys.h"
#include "tcpip.h"
#include "timer.h"
#include "uart.h"
#include "wdt.h"
#include "wifi.h"
