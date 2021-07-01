// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdbool.h>
#include <stdint.h>

void uart_init(int no, int tx, int rx, int baud);
bool uart_read(int no, uint8_t *c);
void uart_write(int no, uint8_t c);
