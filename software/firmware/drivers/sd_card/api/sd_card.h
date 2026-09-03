/// SD card API
#pragma once

#include "system/result_codes.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
  SD_V1_SDSC = 1,
  SD_V2_SDSC = 2,
  SD_V2_SDHC_SDXC = 3,
} SD_CardType_e;

UnicornResult_e SD_Init(void);

SD_CardType_e SD_Type(void);

// Supplied buffer MUST be 512 bytes long
UnicornResult_e SD_ReadBlock(uint32_t block_num, uint8_t *buffer);

// Supplied buffer MUST be at least 512 * num_of_blocks
UnicornResult_e SD_ReadBlocks(uint32_t start_block_num, size_t num_of_blocks, uint8_t *buffer);

// Write the buffer (MUST be 512 bytes long)
UnicornResult_e SD_WriteBlock(uint32_t block_num, const uint8_t *buffer);

UnicornResult_e SD_Sync(void);