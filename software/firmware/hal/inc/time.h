/// Time functions API
#pragma once

#include <stdint.h>

void Time_Init(void);
void Time_Irq(void);
void delay_ms(uint32_t ms);
// Not really accurate here without a faster system clock
void delay_us(uint32_t us);

void delay_4us(void); // around 4.4us?

#define delay_2us() __asm__ volatile("nop"); // No better way then that