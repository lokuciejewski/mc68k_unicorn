#include "../inc/duart.h"
#include "../inc/memory.h"
#include "../inc/serial.h"
#include "../inc/time.h"

#include <stdint.h>

#define DUART_Isr_InpChg (1 << 7)
#define DUART_Isr_BreakB (1 << 6)
#define DUART_Isr_RxRDYFB (1 << 5)
#define DUART_Isr_TxRDYB (1 << 4)
#define DUART_Isr_CntRDY (1 << 3)
#define DUART_Isr_BreakA (1 << 2)
#define DUART_Isr_RxRDYFA (1 << 1)
#define DUART_Isr_TxRDYA (1 << 0)

static volatile uint8_t interrupt_register = 0;

void INTERRUPT DUART_Irq() {
  uint8_t isr = MEM(DUART_ISR);
  if (isr & DUART_Isr_RxRDYFA) {
    Serial_RxIrq(Serial_A);
  } else if (isr & DUART_Isr_RxRDYFB) {
    Serial_RxIrq(Serial_B);
  } else if (isr & DUART_Isr_CntRDY) {
    Time_Irq();
  } else {
    panic("Unhandled DUART Irq");
  }
}

void DUART_EnableIrq(uint8_t mask) {
  interrupt_register |= mask;
  duart_write(DUART_IMR, interrupt_register);
}

void DUART_DisableIrq(uint8_t mask) {
  interrupt_register &= ~mask;
  duart_write(DUART_IMR, interrupt_register);
}