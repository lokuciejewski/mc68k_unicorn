#include "api/sd_card.h"
#include "../../hal/inc/gpio.h"
#include "../../hal/inc/spi.h"
#include "../../hal/inc/time.h"
#include "system/result_codes.h"
#include <stdbool.h>
#include <string.h>

#define SD_ReadByte()                                                          \
  SPI_WriteByteNoSel(0xff) // SD card expects MOSI to be default high

#define SD_WriteByte(byte) SPI_WriteByteNoSel(byte)

// CMD0 - GO_IDLE_STATE
#define CMD0 0
#define CMD0_ARG 0
#define CMD0_CRC 0x94

// CMD8 - SEND_IF_COND (Send Interface Condition)
#define CMD8 8
#define CMD8_ARG 0x0000001AA // 3.3V
#define CMD8_CRC 0x86

// CMD58 - READ_OCR (Read Operation Condition Register)
#define CMD58 58
#define CMD58_ARG 0
#define CMD58_CRC 0 // not needed unless enabled explicitly

// CMD55 - APP_CMD (Informs the SD card that the next command is an
// application-specific command)
#define CMD55 55
#define CMD55_ARG 0
#define CMD55_CRC 0

// ACMD41 - SD_SEND_OP_COND (Send Operating Condition)
#define ACMD41 41
#define ACMD41_ARG 0x40000000
#define ACMD41_CRC 0

// CMD17 - READ_SINGLE_BLOCK
#define CMD17 17
#define CMD17_CRC 0

// CMD18 - READ_MULTIPLE_BLOCK
#define CMD18 18
#define CMD18_CRC 0

// CMD12 - STOP_ALL_TRANSMISSIONS
#define CMD12 12
#define CMD12_CRC 0

// CMD24 - WRITE_SINGLE_BLOCK
#define CMD24 24
#define CMD24_CRC 0

// 100ms timeout while clocking is needed. It takes around 320us for a single
// byte to be sent/read. 100ms is 312.5*320us so to make it safer:
#define SD_MAX_READ_ATTEMPTS 320U
// 250ms timeout while clocking is needed. Using same math as for read, it
// needs 2.5 * 320 attempts, so:
#define SD_MAX_WRITE_ATTEMPTS 800U

#define SD_START_BLOCK 0xfe
#define SD_BLOCK_SIZE 512U

typedef enum {
  SD_Response1,
  SD_Response3,
  SD_Response7,
} SD_Response_e;

static void SD_Select(void) {
  GPIO_Led_Set(Led_3, true);
  SD_ReadByte(); // Send 8 clocks before and after CS to ensure the
                 // SD card is selected
  SPI_Select(true);
  SD_ReadByte();
}

static void SD_Deselect(void) {
  SD_ReadByte(); // Send 8 clocks before and after CS to ensure the
                 // SD card is deselected
  SPI_Select(false);
  SD_ReadByte();
  GPIO_Led_Set(Led_3, false);
}

static void SD_SendCommand(uint8_t command, uint32_t arg, uint8_t crc) {
  SD_WriteByte(command |
               0x40); // SD commands are "prefixed" on the 2 MS bits with 0b01

  SD_WriteByte((uint8_t)(arg >> 24));
  SD_WriteByte((uint8_t)(arg >> 16));
  SD_WriteByte((uint8_t)(arg >> 8));
  SD_WriteByte((uint8_t)(arg));

  SD_WriteByte(crc | 0b1); // SD command must end with 1
}

static void SD_ReadResponse1(uint8_t *response) {
  for (uint8_t i = 0; i < 16; i++) {
    response[0] = SD_ReadByte();
    if (response[0] != 0xff) {
      return;
    }
  }
  response[0] = 0xff;
}

static void SD_ReadResponse(SD_Response_e resp, uint8_t *response) {
  switch (resp) {
  case SD_Response1:
    SD_ReadResponse1(response);
    break;
  case SD_Response3:
  case SD_Response7:
    SD_ReadResponse1(response);
    if (response[0] > 0x1) {
      memset(&response[1], 0, 4);
      return;
    }
    for (uint8_t i = 1; i <= 4; i++) {
      response[i] = SD_ReadByte();
    }
    break;
  default:
    break;
  }
}

static UnicornResult_e SD_SendCommand_ACMD41(uint8_t *response,
                                             SD_CardType_e *card_type) {
  for (uint8_t i = 0; i < 100; i++) {
    SD_Select();
    SD_SendCommand(CMD55, CMD55_ARG, CMD55_CRC);
    SD_ReadResponse(SD_Response1, response);
    SD_Deselect();

    SD_Select();
    if (*card_type == SD_V1_SDSC) {
      SD_SendCommand(ACMD41, 0, ACMD41_CRC);
    } else {
      SD_SendCommand(ACMD41, ACMD41_ARG, ACMD41_CRC);
    }
    SD_ReadResponse(SD_Response1, response);
    SD_Deselect();
    if (response[0] == 0x00) {
      return UniRes_Ok;
    }
    delay_ms(10);
  }

  return UniRes_Timeout;
}

static SD_CardType_e card_type = 0;

UnicornResult_e SD_Init(void) {
  SD_Deselect();

  for (uint8_t i = 0; i < 10; i++) {
    SD_ReadByte(); // Send 80 clock cycles to synchronise, card must
                   // be deselected
  }

  uint8_t sd_response[5] = {0};
  uint8_t attempts = 0;
  while (sd_response[0] != 0x01) {
    SD_Select();
    SD_SendCommand(CMD0, CMD0_ARG, CMD0_CRC);
    SD_ReadResponse(SD_Response1, sd_response);
    SD_Deselect();
    attempts++;
    if (attempts > 10) {
      return UniRes_Timeout;
    }
  }

  SD_Select();
  SD_SendCommand(CMD8, CMD8_ARG, CMD8_CRC);
  SD_ReadResponse(SD_Response7, sd_response);
  SD_Deselect();
  if (sd_response[0] == 0x01) {
    // The SD card is version >=2.00
    // Check voltage response
    if (sd_response[3] != 0x01) {
      return UniRes_SD_IncompatibleVoltageRange;
    }

    // Check echoed pattern
    if (sd_response[4] != 0xaa) {
      return UniRes_SD_CheckPatternMismatch;
    }

    SD_Select();
    SD_SendCommand(CMD58, CMD58_ARG, CMD58_CRC);
    SD_ReadResponse(SD_Response3,
                    sd_response); // Response ignored but must be read
    SD_Deselect();

    UnicornResult_e result = SD_SendCommand_ACMD41(sd_response, &card_type);
    if (result != UniRes_Ok) {
      return result;
    }

    SD_Select();
    SD_SendCommand(CMD58, CMD58_ARG, CMD58_CRC);
    SD_ReadResponse(SD_Response3,
                    sd_response); // Now response contains valid data
    SD_Deselect();

    if (!(sd_response[1] & 0x80)) {
      return UniRes_SD_PowerUpBitNotSet;
    }

    if (sd_response[1] & 0x40) {
      card_type = SD_V2_SDHC_SDXC;
    } else {
      card_type = SD_V2_SDSC;
    }

  } else if (sd_response[0] == 0x05) {
    // Illegal command indicates the SD card if first gen or MMC
    card_type = SD_V1_SDSC;
    SD_SendCommand_ACMD41(sd_response, &card_type);
    if (sd_response[0] & 0b100) {
      return UniRes_SD_NotSDCard;
    }
    if (sd_response[0] != UniRes_Ok) {
      return UniRes_Timeout;
    }
  } else {
    return UniRes_GeneralError;
  }

  return UniRes_Ok;
}

SD_CardType_e SD_Type(void) { return card_type; }

// `ops` should be `SD_MAX_READ_ATTEMPTS` or `SD_MAX_WRITE_ATTEMPTS`
static uint8_t SD_WaitForReady(uint16_t ops) {
  uint8_t read = 0xff;
  for (uint16_t i = 0; i < ops; i++) {
    read = SD_ReadByte();
    if (read != 0xff) {
      return read;
    }
  }
  return 0xff;
}

UnicornResult_e SD_ReadBlock(uint32_t block_num, uint8_t *buffer) {
  uint8_t sd_response = 0xff;

  if (card_type == SD_V1_SDSC) {
    block_num *= 512; // SDSC is byte-addressed, the rest are block-addressed
  }

  SD_Select();
  SD_SendCommand(CMD17, block_num, CMD17_CRC);
  SD_ReadResponse(SD_Response1, &sd_response);
  if (sd_response == 0x00) {

    // Wait up to 100ms for the "Start Block" response
    uint8_t read = SD_WaitForReady(SD_MAX_READ_ATTEMPTS);

    if (read == SD_START_BLOCK) {
      for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
        buffer[i] = SD_ReadByte();
      }

      SD_ReadByte();
      SD_ReadByte(); // Read 16-bit CRC
    } else {
      sd_response = UniRes_GeneralError;
    }
  }

  SD_Deselect();
  return sd_response;
}

UnicornResult_e SD_ReadBlocks(uint32_t start_block_num, size_t num_of_blocks,
                              uint8_t *buffer) {
  uint8_t sd_response = 0xff;

  if (card_type == SD_V1_SDSC) {
    start_block_num *=
        512; // SDSC is byte-addressed, the rest are block-addressed
  }

  SD_Select();
  SD_SendCommand(CMD18, start_block_num, CMD18_CRC);
  SD_ReadResponse(SD_Response1, &sd_response);
  if (sd_response == 0x00) {
    for (size_t block = 0; block < num_of_blocks; block++) {
      // Wait up to 100ms for the "Start Block" response
      uint8_t read = SD_WaitForReady(SD_MAX_READ_ATTEMPTS);
      if (read == SD_START_BLOCK) {
        for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
          buffer[(block * SD_BLOCK_SIZE) + i] = SD_ReadByte();
        }

        SD_ReadByte();
        SD_ReadByte(); // Read 16-bit CRC
      } else {
        sd_response = UniRes_GeneralError;
        break;
      }
    }
    SD_SendCommand(CMD12, 0, CMD12_CRC);
    uint8_t cmd12_response;
    SD_ReadResponse(SD_Response1, &cmd12_response);
  } else {
    sd_response = UniRes_GeneralError;
  }

  SD_Deselect();
  return sd_response;
}

UnicornResult_e SD_WriteBlock(uint32_t block_num, const uint8_t *buffer) {
  uint8_t sd_response = 0xff;

  if (card_type == SD_V1_SDSC) {
    block_num *= 512; // SDSC is byte-addressed, the rest are block-addressed
  }

  SD_Select();
  SD_SendCommand(CMD24, block_num, CMD24_CRC);
  SD_ReadResponse(SD_Response1, &sd_response);
  if (sd_response == 0) {
    SD_WriteByte(SD_START_BLOCK);

    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
      SD_WriteByte(buffer[i]);
    }

    // Dummy CRC
    SD_WriteByte(0xff);
    SD_WriteByte(0xff);

    uint8_t data_resp = SD_WaitForReady(SD_MAX_WRITE_ATTEMPTS);

    if ((data_resp & 0x1F) != 0x05) {
      SD_Deselect();
      return UniRes_GeneralError;
    }

    SD_Deselect();
    return UniRes_Ok;
  }
  SD_Deselect();
  return UniRes_GeneralError;
}

UnicornResult_e SD_Sync(void) {
  SD_Select();
  uint8_t data_resp = SD_WaitForReady(SD_MAX_WRITE_ATTEMPTS);
  if (data_resp != 0xff) {
    SD_Deselect();
    return UniRes_Ok;
  }
  SD_Deselect();
  return UniRes_GeneralError;
}