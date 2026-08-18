#include "../inc/time.h"
#include "../inc/duart.h"
#include "../inc/memory.h"

const uint8_t Timer_Irq_Mask = 0b1000;
static volatile uint32_t timer = 0;

void Time_Init(void) {
  // Set ACR[6:4] = 110 (Timer Mode, X1/CLK source)
  // Preserving ACR[7] (Baud rate set) and ACR[3:0]
  uint8_t current_acr = MEM(DUART_ACR);
  MEM(DUART_ACR) = (current_acr | 0b01100000); // External clock reference

  // Load 16-bit count value 0x0733 (~1 ms at 1.8432MHz)
  MEM(DUART_CUR) = 0x01; // Upper byte
  MEM(DUART_CLR) = 0xcd; // Lower byte

  [[maybe_unused]] volatile uint8_t t = MEM(DUART_START_CNT); // Start the timer
}

void Time_Irq(void) {
  timer++;
  [[maybe_unused]] volatile uint8_t t =
      MEM(DUART_STOP_CNT); // Reset the ISR flag (does not stop the timer)
}

void delay_ms(uint32_t ms) {
  DUART_EnableIrq(Timer_Irq_Mask);
  volatile uint32_t current_timer = timer;
  while (timer < current_timer + ms) {
  }
  DUART_DisableIrq(Timer_Irq_Mask);
}

void __attribute__((noinline)) delay_4us(void) {
  return;
}

void delay_us(uint32_t us) {
  if (us == 0)
    return;

  /* Scale the loops by a known factor */
  unsigned long loops = us / 100UL;

  if (loops == 0)
    return;

  /* DBF counts down to -1 */
  loops--;

  __asm__ volatile("1: dbf %0,1b" : "+d"(loops) : : "cc");
}