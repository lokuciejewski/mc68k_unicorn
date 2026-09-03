#include "../inc/spi.h"
#include "../inc/duart.h"
#include "../inc/memory.h"
#include "../inc/time.h"
#include <stdint.h>

#define SCLK (1 << 5) // OP5
#define MOSI (1 << 6) // OP6
#define SS (1 << 7)   // OP7
#define MISO (1 << 2) // IP2

static inline void select(void) { MEM(DUART_OPR_SET) = SS; }

static inline void deselect(void) { MEM(DUART_OPR_RESET) = SS; }

static inline void clk(uint8_t state) {
  if (state) {
    MEM(DUART_OPR_RESET) = SCLK;
  } else {
    MEM(DUART_OPR_SET) = SCLK;
  }
}

static inline void mosi(uint8_t state) {
  if (state) {
    MEM(DUART_OPR_RESET) = MOSI;
  } else {
    MEM(DUART_OPR_SET) = MOSI;
  }
}

void SPI_Init(void) {
  clk(1);
  mosi(1);
  deselect();
}

uint8_t SPI_WriteByteNoSel(uint8_t byte) {
  uint8_t read = 0;
// Unrolled loop read for faster speeds
#define BIT_X(n)                                                               \
  if ((byte >> n) & 1)                                                         \
    MEM(DUART_OPR_RESET) = MOSI;                                               \
  else                                                                         \
    MEM(DUART_OPR_SET) = MOSI;                                                 \
  MEM(DUART_OPR_RESET) = SCLK;                                                 \
  if (MEM(DUART_IP) & MISO)                                                    \
    read |= (1 << n);                                                          \
  MEM(DUART_OPR_SET) = SCLK;

  BIT_X(7);
  BIT_X(6);
  BIT_X(5);
  BIT_X(4);
  BIT_X(3);
  BIT_X(2);
  BIT_X(1);
  BIT_X(0);

#undef BIT_X
  return read;
}

uint8_t SPI_WriteByte(uint8_t byte) {
  select();
  uint8_t read_byte = SPI_WriteByteNoSel(byte);
  deselect();
  return read_byte;
}

uint8_t SPI_ReadByteNoSel(void) { return SPI_WriteByteNoSel(0); }

uint8_t SPI_ReadByte(void) { return SPI_WriteByte(0); }

void SPI_Select(bool state) {
  if (state) {
    select();
  } else {
    deselect();
  }
}