#pragma once

#if __has_include("ushell_cfg.h")
#include "ushell_cfg.h"
#else
#error "uShell needs a `ushell_cfg.h` header with proper definitions to work"
#endif

int ushell(void);