// Syscalls (trap #15) definitions for the Unicorn SBC
#pragma once

// Increment any time the api changes
#include <stdint.h>
#define TRAP_API_VERSION 0x00000001
#define FD_FIRST_FOR_HAL 0
#define FD_LAST_FOR_HAL 2
#define FD_FIRST_FOR_FS 3

typedef enum : uint32_t {
  // Version call
  Syscall_Version,
  // Serial
  Syscall_Serial_Init,
  Syscall_Serial_Putc,
  Syscall_Serial_Getc,
  Syscall_Serial_GetcT,
  Syscall_Serial_PrintStr,
  Syscall_Serial_SetStdio,
  // Time
  Syscall_Time_DelayMs,
  Syscall_Time_Delay2us,
  // GPIO
  Syscall_GPIO_Led_Set,
  // SD Card
  Syscall_SD_Init,
  Syscall_SD_ReadBlock,
  Syscall_SD_WriteBlock,
  Syscall_SD_Sync,
  // POSIX
  Syscall_Write,
  Syscall_Read,
  Syscall_Open,
  Syscall_Close,
  Syscall_Lseek,
  Syscall_Exit,
  Syscall_Putc,
  Syscall_Getc

} SyscallFunc_e;
