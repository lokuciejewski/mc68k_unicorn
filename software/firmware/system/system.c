#include "system.h"
#include "../hal/inc/serial.h"
#include "../hal/inc/time.h"
#include "ff.h"
#include "gpio.h"
#include "sd_card.h"
#include "syscalls.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>

extern int tinylisp();

static volatile Serial_Instance_e current_instance = Serial_A;

static void Serial_SetStdio(Serial_Instance_e serial) {
  current_instance = serial;
}

static void t15_exit() {
  tinylisp();
  while (1) {
  }
}

static size_t t15_write(int fd, const uint8_t *buf, size_t count) {
  switch (fd) {
  case 0: // stdout
  case 1: // stdin
  case 2: // stderr
    return Serial_WriteBytes(Serial_A, buf, count);
  default:
    // invalid fd
    break;
  }
  return 0;
}

static size_t t15_read(int fd, uint8_t *buf, size_t count) {
  switch (fd) {
  case 0: // stdout
  case 1: // stdin
  case 2: // stderr
    return Serial_ReadBytes(Serial_A, buf, count);
  default:
    // invalid fd
    break;
  }
  return 0;
}

static int t15_open([[maybe_unused]] const char *path,
                    [[maybe_unused]] int mode) {
  return 0;
}

static int t15_close([[maybe_unused]] int fd) { return 0; }

static off_t t15_lseek([[maybe_unused]] int fd, [[maybe_unused]] off_t offset,
                       [[maybe_unused]] int whence) {
  return 0;
}

uint32_t Trap15_Handler(uint32_t func_id, uint32_t arg1, uint32_t arg2,
                        uint32_t arg3) {
  switch ((SyscallFunc_e)func_id) {
  case Syscall_Version:
    return TRAP_API_VERSION;
  case Syscall_Serial_Init: {
    const Serial_Settings_t *settings = (const Serial_Settings_t *)arg1;
    Serial_Init(settings);
    break;
  }
  case Syscall_Serial_Putc: {
    Serial_Instance_e inst = (Serial_Instance_e)arg1;
    char c = (char)(arg2);
    Serial_Putc((const Serial_Instance_e)inst, c);
    break;
  }
  case Syscall_Serial_Getc: {
    Serial_Instance_e inst = (Serial_Instance_e)arg1;
    return Serial_Getc((const Serial_Instance_e)inst);
  }
  case Syscall_Serial_GetcT: {
    Serial_Instance_e inst = (Serial_Instance_e)arg1;
    uint16_t timeout_ms = (uint16_t)arg2;
    char *c = (char *)arg3;
    return Serial_GetcT((const Serial_Instance_e)inst, timeout_ms, c);
  }
  case Syscall_Serial_PrintStr: {
    Serial_Instance_e inst = (Serial_Instance_e)arg1;
    const char *c = (const char *)(arg2);
    Serial_PrintStr((const Serial_Instance_e)inst, (const char *)c);
    break;
  }
  case Syscall_Serial_SetStdio: {
    Serial_Instance_e inst = (Serial_Instance_e)arg1;
    Serial_SetStdio((const Serial_Instance_e)inst);
    break;
  }
  case Syscall_Time_DelayMs: {
    uint32_t ms = arg1;
    delay_ms(ms);
    break;
  }
  case Syscall_Time_Delay2us: {
    delay_2us();
    break;
  }
  case Syscall_GPIO_Led_Set: {
    GPIO_Led_e led = (GPIO_Led_e)arg1;
    bool state = (bool)arg2;
    GPIO_Led_Set(led, state);
    break;
  }
  case Syscall_SD_Init:
    return SD_Init();
  case Syscall_SD_ReadBlock: {
    uint32_t block_num = arg1;
    uint8_t *buffer = (uint8_t *)arg2;
    return SD_ReadBlock(block_num, buffer);
  }
  case Syscall_SD_WriteBlock: {
    uint32_t block_num = arg1;
    const uint8_t *buffer = (const uint8_t *)arg2;
    return SD_WriteBlock(block_num, buffer);
  }
  case Syscall_SD_Sync:
    return SD_Sync();
  // POSIX syscalls
  case Syscall_Putc:
    char c = (char)arg1;
    Serial_Putc(current_instance, c);
    break;
  case Syscall_Getc:
    return Serial_Getc(current_instance);
  case Syscall_Exit:
    t15_exit();
    break;
  case Syscall_Open:
    return t15_open((const char *)arg1, (int)arg2);
  case Syscall_Close:
    return t15_close((int)arg1);
  case Syscall_Lseek:
    return t15_lseek((int)arg1, (off_t)arg2, (int)arg3);
  case Syscall_Write: {
    int fd = (int)arg1;
    const uint8_t *buf = (const uint8_t *)arg2;
    size_t count = (size_t)arg3;
    return t15_write(fd, buf, count);
  }
  case Syscall_Read: {
    int fd = (int)arg1;
    uint8_t *buf = (uint8_t *)arg2;
    size_t count = (size_t)arg3;
    return t15_read(fd, buf, count);
  }
  }

  return 0;
}

static FIL *files[MAX_CONCURRENTLY_OPEN_FILES] = {nullptr};

static int t14_open([[maybe_unused]] const char *path,
                    [[maybe_unused]] int mode) {
  int first_empty = -1;
  for (int i = 0; i < (int)MAX_CONCURRENTLY_OPEN_FILES; i++) {
    if (files[i] == nullptr) {
      first_empty = i;
      break;
    }
  }
  if (first_empty == -1) {
    return first_empty; // Too many open files
  }
  files[first_empty] = malloc(sizeof(FIL));
  FRESULT res = f_open(files[first_empty], path, mode);
  if (res != FR_OK) {
    free(files[first_empty]);
    files[first_empty] = nullptr;
    return -res;
  }
  return first_empty + FD_FIRST_FOR_FS;
}

static int t14_close([[maybe_unused]] int fd) {
  size_t idx = (size_t)fd - FD_FIRST_FOR_FS;
  FRESULT res = f_close(files[idx]); // TODO: convert to UnicornResult??
  if (res == FR_OK) {
    free(files[idx]);
    files[idx] = nullptr;
  }
  return res;
}

static off_t t14_lseek([[maybe_unused]] int fd, [[maybe_unused]] off_t offset,
                       [[maybe_unused]] int whence) {
  return 0;
}
static size_t t14_write(int fd, const uint8_t *buf, size_t count) {
  switch (fd) {
  case 0:     // stdout
  case 1:     // stdin
  case 2:     // stderr
    return 0; // invalid fd
  default: {
    size_t idx = fd - FD_FIRST_FOR_FS;
    UINT bytes_written = 0;
    (void)f_write(files[idx], buf, count, &bytes_written);
    (void)f_sync(files[idx]); // TODO: is this necessary?
    return bytes_written;
  }
  }
  return 0;
}

static size_t t14_read(int fd, uint8_t *buf, size_t count) {
  switch (fd) {
  case 0:     // stdout
  case 1:     // stdin
  case 2:     // stderr
    return 0; // invalid fd
  default: {
    size_t idx = fd - FD_FIRST_FOR_FS;
    UINT bytes_read = 0;
    (void)f_read(files[idx], buf, count, &bytes_read);
    return bytes_read;
  }
  }
  return 0;
}

uint32_t Trap14_Handler(uint32_t func_id, uint32_t arg1, uint32_t arg2,
                        uint32_t arg3) {

  switch ((SyscallFunc_e)func_id) {
  case Syscall_Version:
    return TRAP_API_VERSION;
  case Syscall_Serial_Init:
  case Syscall_Serial_Putc:
  case Syscall_Serial_Getc:
  case Syscall_Serial_GetcT:
  case Syscall_Serial_PrintStr:
  case Syscall_Serial_SetStdio:
  case Syscall_Time_DelayMs:
  case Syscall_Time_Delay2us:
  case Syscall_GPIO_Led_Set:
  case Syscall_SD_Init:
  case Syscall_SD_ReadBlock:
  case Syscall_SD_WriteBlock:
  case Syscall_SD_Sync:
  case Syscall_Exit:
  case Syscall_Putc:
  case Syscall_Getc:
    break;
  case Syscall_Open:
    return t14_open((const char *)arg1, (int)arg2);
  case Syscall_Close:
    return t14_close((int)arg1);
  case Syscall_Lseek:
    return t14_lseek((int)arg1, (off_t)arg2, (int)arg3);
  case Syscall_Write: {
    int fd = (int)arg1;
    const uint8_t *buf = (const uint8_t *)arg2;
    size_t count = (size_t)arg3;
    return t14_write(fd, buf, count);
  }
  case Syscall_Read: {
    int fd = (int)arg1;
    uint8_t *buf = (uint8_t *)arg2;
    size_t count = (size_t)arg3;
    return t14_read(fd, buf, count);
  }
  }

  return 0;
}