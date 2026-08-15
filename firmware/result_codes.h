/// Result codes used in the low level firmware
#pragma once

#include <stdint.h>

typedef enum : uint8_t {
  // General codes
  UniRes_Ok,
  UniRes_GeneralError,
  UniRes_Timeout,
  UniRes_InvalidPtr,

  UniRes_NumOf,
} UnicornResult_e;