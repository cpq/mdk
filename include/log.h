// Copyright (c) 2021 Cesanta
// All rights reserved

#pragma once

#include <stdarg.h>

void sdk_log(char *fmt, ...);
void sdk_vlog(char *fmt, va_list);
