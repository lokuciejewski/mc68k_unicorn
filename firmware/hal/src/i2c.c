#include "../inc/i2c.h"

void I2C_Init(void) {}

UnicornResult_e I2C_WriteByte(uint8_t byte) {}
UnicornResult_e I2C_WriteBytes(uint8_t *bytes, uint8_t count) {}
UnicornResult_e I2C_WriteBytesA(uint8_t address, uint8_t reg, uint8_t *bytes,
                                uint8_t count) {}

UnicornResult_e I2C_ReadByte(uint8_t *byte) {}
UnicornResult_e I2C_ReadBytes(uint8_t *bytes, uint8_t count) {}
UnicornResult_e I2C_ReadBytesA(uint8_t address, uint8_t *bytes, uint8_t count) {
}