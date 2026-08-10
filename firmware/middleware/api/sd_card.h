/// SD card API
#pragma once

#include <stdint.h>
typedef enum {
  SD_OK,
  SD_IDLE_STATE_TIMEOUT,
  SD_GENERAL_ERROR,
  SD_CHECK_PATTERN_MISMATCH,
  SD_INCOMPATIBLE_VOLTAGE_RANGE,
  SD_POWER_UP_BIT_NOT_SET,
  SD_NOT_SD_CARD
} SD_RetCode_e;

typedef enum {
  SD_V1_SDSC = 1,
  SD_V2_SDSC = 2,
  SD_V2_SDHC_SDXC = 3,
} SD_CardType_e;

SD_RetCode_e SD_Init(void);

SD_CardType_e SD_Type(void);

// Supplied buffer MUST be 512 bytes long
SD_RetCode_e SD_ReadBlock(uint32_t block_num, uint8_t *buffer);

// Write the buffer (MUST be 512 bytes long)
SD_RetCode_e SD_WriteBlock(uint32_t block_num, const uint8_t *buffer);

SD_RetCode_e SD_Sync(void);