// Syscalls API
#pragma once

#include <stdint.h>

#define MAX_CONCURRENTLY_OPEN_FILES 64U

/// Trap #14 handler responsible for calling FATFS functions. Called from
/// assembly (trap_handler.s)
uint32_t Trap14_Handler(uint32_t func_id, uint32_t arg1, uint32_t arg2,
                        uint32_t arg3);

/// Trap #15 handler responsible for calling Unicorn Firmware functions from
/// HAL/Drivers. Called from assembly (trap_handler.s)
uint32_t Trap15_Handler(uint32_t func_id, uint32_t arg1, uint32_t arg2,
                        uint32_t arg3);