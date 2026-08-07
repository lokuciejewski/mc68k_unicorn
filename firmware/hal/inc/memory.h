#pragma once

#include <stdint.h>

#define MEM(address) (*(volatile uint8_t *)((uintptr_t)address))
#define MEM16(address) (*(volatile uint16_t *)((uintptr_t)address))
#define MEM32(address) (*(volatile uint32_t *)((uintptr_t)address))

/**
 * Used to avoid clbr since on 68010 it performs implicit read which messes up
 * with the baudrate scaling
 */
inline void __attribute__((always_inline)) duart_write(uint32_t addr,
                                                       uint8_t val) {
  __asm__ volatile("move.b %0, (%1)" : : "d"(val), "a"(addr) : "memory");
}