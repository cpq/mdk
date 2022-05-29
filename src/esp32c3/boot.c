// Copyright (c) 2021 Cesanta
// All rights reserved

#include "mdk.h"

extern void start(void);
void _reset(void) {
  start();
  for (;;) spin(0);
}
