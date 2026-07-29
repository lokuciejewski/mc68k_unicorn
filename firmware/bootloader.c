#include "hal/inc/gpio.h"
#include "hal/inc/irq.h"
#include "hal/inc/serial.h"

#include "ushell/ushell.h"

int main();

static volatile uint8_t USER_STACK[32768] = {0};

inline static void __attribute__((always_inline)) enter_user_mode(void) {
  __asm__ volatile("move.l %0, %%usp\n\t" // Set user stack
                   "and.w #0xdfff, %%sr"  // Enter user mode
                   :
                   : "a"(USER_STACK + 32768 - 4)
                   : "memory", "cc");
}

void _start(void) {
  Irq_SetInterrupts(false);
  Irq_Init();
  Serial_Settings_t settings = {.instance = Serial_A,
                                .baudrate = Serial_Baudrate_19200,
                                .bit_conf = Serial_8n,
                                .stop_bits = Serial_StopBits1,
                                .rx_rts = false,
                                .tx_rts = false,
                                .cts = false};
  Serial_Init(&settings);
  GPIO_Init();
  GPIO_Led_Set(Led_1, true);
  Irq_SetInterrupts(true);
  GPIO_Led_Set(Led_2, true);
  main();
}

int main() {
  enter_user_mode();
  GPIO_Led_Set(Led_3, true);
  Serial_PrintStr(Serial_A, "\r\nUnicorn SBC v1\r\n");

  return ushell();
}
