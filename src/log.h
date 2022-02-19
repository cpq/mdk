// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdarg.h>
#include <stddef.h>

// void log(const char *fmt, ...);
// void vlog(const char *fmt, va_list);
void hexdump(const void *buf, size_t len);
