/// Software SPI API
#pragma once

#include <stdbool.h>
#include <stdint.h>


void SPI_Init(void);
uint8_t SPI_WriteByteNoSel(uint8_t byte);
uint8_t SPI_WriteByte(uint8_t byte);
uint8_t SPI_ReadByteNoSel(void);
uint8_t SPI_ReadByte(void);

void SPI_Select(bool state);