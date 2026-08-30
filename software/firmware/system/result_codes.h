/// Result codes used in the low level firmware
#pragma once

#include <stdint.h>

typedef enum : uint8_t {
  // General codes
  UniRes_Ok,
  UniRes_GeneralError,
  UniRes_Timeout,
  UniRes_InvalidPtr,
  UniRes_NotImplemented,

  // SD card codes
  UniRes_SD_CheckPatternMismatch,
  UniRes_SD_IncompatibleVoltageRange,
  UniRes_SD_PowerUpBitNotSet,
  UniRes_SD_NotSDCard,

  // Bootloader codes
  UniRes_InvalidElf,

  // FS codes
  UniRes_NoSuchFile,

  UniRes_NumOf,
} UnicornResult_e;