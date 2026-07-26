#include "hal/inc/gpio.h"
#include "hal/inc/irq.h"
#include "hal/inc/serial.h"

#include "ushell/ushell.h"

int main();

extern uint8_t USER_STACK_TOP[];

inline static void __attribute__((always_inline)) enter_user_mode(void) {
  __asm__ volatile("move.l %0, %%usp\n\t" // Set user stack
                   "and.w #0xdfff, %%sr"  // Enter user mode
                   :
                   : "a"(USER_STACK_TOP)
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
  Irq_SetInterrupts(true);
  enter_user_mode();
  main();
}

int main() {
  GPIO_Led_Set(Led_1, true);
  const Serial_Instance_e s = Serial_A;
  Serial_PrintStr(s, "\r\nUnicorn SBC v1\r\n");
  GPIO_Led_Set(Led_2, true);

  return ushell();
}
