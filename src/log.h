// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdarg.h>
#include <stddef.h>

void sdk_log(const char *fmt, ...);
void sdk_vlog(const char *fmt, va_list);
void sdk_hexdump(const void *buf, size_t len);
