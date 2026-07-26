#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum : uint8_t {
  Led_1 = 0b100,
  Led_2 = 0b1000,
  Led_3 = 0b10000,
} GPIO_Led_e;

void GPIO_Init(void);
void GPIO_Led_Set(GPIO_Led_e led, bool high);
bool GPIO_Get(void);