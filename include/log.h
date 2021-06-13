// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdarg.h>

void sdk_log(const char *fmt, ...);
void sdk_vlog(const char *fmt, va_list);
