#include "../inc/gpio.h"
#include "../inc/duart.h"
#include "../inc/memory.h"


void GPIO_Init(void) {
  MEM(DUART_OPCR) =
      0x00; // OP0/OP1 = RTSA//RTSB/, OP2-OP7 = general-purpose GPIO
  MEM(DUART_OPR_SET) = 0b00011100; // Clear output LEDs
}

void GPIO_Led_Set(GPIO_Led_e led, bool high) {
  if (high) {
    MEM(DUART_OPR_RESET) = (uint8_t)led;
  } else {
    MEM(DUART_OPR_SET) = (uint8_t)led;
  }
}