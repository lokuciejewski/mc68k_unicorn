#include "system.h"
#include "../hal/inc/serial.h"
#include "../hal/inc/time.h"
#include "gpio.h"
#include "irq.h"
#include "sd_card.h"
#include "syscalls.h"

static volatile Serial_Instance_e current_instance = Serial_A;

static void Serial_SetStdio(Serial_Instance_e serial) {
  current_instance = serial;
}

void __attribute__((noreturn)) exit() {
  panic("Firmware exit");
  while (1) {
  }
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
  case Syscall_Write:

  case Syscall_Read:

  case Syscall_Putc:
    char c = (char)arg1;
    Serial_Putc(current_instance, c);
    break;
  case Syscall_Getc:
    return Serial_Getc(current_instance);
  case Syscall_Exit:
    exit();
    break;
  }
  return 0;
}