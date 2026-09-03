/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "api/diskio.h" /* Declarations FatFs MAI */
#include "../../drivers/sd_card/api/sd_card.h"
#include "api/ff.h" /* Basic definitions of FatFs */
#include <stdbool.h>

typedef enum {
  SD_Card,
} DriveID_e;

static bool sd_card_initialized = false;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
  switch (pdrv) {
  case SD_Card:
    if (sd_card_initialized) {
      return 0;
    } else {
      return STA_NOINIT;
    }
  default:
    return STA_NODISK;
  }
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
  switch (pdrv) {
  case SD_Card:
    switch (SD_Init()) {
    case UniRes_Ok:
      sd_card_initialized = true;
      return 0; // OK
    default:
      return STA_NODISK;
    }
    break;
  default:
    return STA_NODISK;
  }
  return STA_NODISK;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
) {
  switch (pdrv) {
  case SD_Card: {
    if (count == 1) {
      if (SD_ReadBlock(sector, buff) != UniRes_Ok) {
        return RES_ERROR;
      }
    } else {
      if (SD_ReadBlocks(sector, count, buff)) {
        return RES_ERROR;
      }
    }
    return RES_OK;
  } break;
  default:
    return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
) {
  switch (pdrv) {
  case SD_Card: {
    for (UINT i = 0; i < count; i++) {
      if (SD_WriteBlock(sector + i, &buff[512 * i]) != UniRes_Ok) {
        return RES_ERROR;
      }
    }
    return RES_OK;
  } break;
  default:
    return RES_ERROR;
  }
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
) {
  switch (pdrv) {
  case SD_Card: {
    if (!sd_card_initialized) {
      return RES_NOTRDY;
    }
    switch (cmd) {
    case CTRL_SYNC:
      SD_Sync();
      return RES_OK;
    case GET_SECTOR_SIZE:
      *(WORD *)buff = 512;
      return RES_OK;
    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1; // Erase block size in sectors
      return RES_OK;
    default:
      return RES_PARERR;
    }
    return RES_ERROR;
  } break;
  default:
    return RES_ERROR;
  }
}
