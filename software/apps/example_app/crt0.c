#include <stdint.h>

extern uint8_t __stack[];
extern int main(int, char **);
extern void __libc_init_array(void);

static void return_to_monitor(void) {
  register uint32_t d0 __asm__("d0") = 19; // Syscall_Exit
  __asm__ volatile("trap #15" : "+r"(d0) : : "memory");
}

void _start(void) {
  __libc_init_array();

  // No need to copy .data or .bss since bootloader did that already

  // Switch to user mode with the app’s stack
  __asm__ volatile("move.l %0, %%usp\n\t"
                   "and.w  #0xdfff, %%sr"
                   :
                   : "a"(__stack)
                   : "memory", "cc");

  (void)main(0, 0);
  return_to_monitor();
}