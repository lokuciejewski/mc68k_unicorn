/// Software SPI API
#pragma once

#include <stdint.h>


void SPI_Init(void);
uint8_t SPI_WriteByte(uint8_t byte);
uint8_t SPI_ReadByte(void);