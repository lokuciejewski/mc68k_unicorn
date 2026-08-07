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

static inline uint8_t miso(void) {
  return (uint8_t)(MEM(DUART_IP & MISO) != 0);
}

void SPI_Init(void) {
  //
}

uint8_t SPI_WriteByte(uint8_t byte) {
  uint8_t read_byte = 0;
  select();
  clk(0);

  for (uint8_t i = 0; i < 8; i++) {
    mosi((byte >> i) & 0b1);
    clk(1);
    if (miso()) {
      read_byte |= (1 << i);
    }
    delay_2us();
    clk(0);
    delay_2us();
  }

  deselect();
  return read_byte;
}

uint8_t SPI_ReadByte(void) { return SPI_WriteByte(0); }