// Syscalls API
#pragma once

#include <stdint.h>

/// Trap #15 handler responsible for calling Unicorn Firmware functions from
/// HAL/Drivers. Called from assembly (trap_handler.s)
uint32_t Trap15_Handler(uint32_t func_id, uint32_t arg1, uint32_t arg2,
                        uint32_t arg3);