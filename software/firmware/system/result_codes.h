/// Result codes used in the low level firmware
#pragma once

#include <stdint.h>

typedef enum : uint8_t {
  // General codes
  UniRes_Ok,
  UniRes_GeneralError,
  UniRes_Timeout,
  UniRes_InvalidPtr,
  UniRes_InvalidData,
  UniRes_InvalidHeader,
  UniRes_NotImplemented,
  UniRes_NotFound,
  UniRes_Skipped,

  // SD card codes
  UniRes_SD_CheckPatternMismatch,
  UniRes_SD_IncompatibleVoltageRange,
  UniRes_SD_PowerUpBitNotSet,
  UniRes_SD_NotSDCard,

  UniRes_NumOf,
} UnicornResult_e;