#include "../inc/irq.h"
#include "../inc/duart.h"
#include "../inc/memory.h"

extern uint8_t IRQ_TABLE[];

#define EXC_PARAM

static void INTERRUPT exception_unhandled(EXC_PARAM) {
  panic("Unhandled exception");
}
static void INTERRUPT exception_bus_error(EXC_PARAM) { panic("Bus error"); }
static void INTERRUPT exception_addr_error(EXC_PARAM) {
  panic("Address error");
}
static void INTERRUPT exception_illegal_inst(EXC_PARAM) {
  panic("Illegal instruction");
}
static void INTERRUPT exception_div_zero(EXC_PARAM) { panic("Divide by zero"); }
static void INTERRUPT exception_chk(EXC_PARAM) {
  panic("Chk instruction out of bounds");
}
static void INTERRUPT exception_trapv(EXC_PARAM) { panic("Trap v"); }
static void INTERRUPT exception_privilege_violation(EXC_PARAM) {
  panic("Privilege violation");
}
static void INTERRUPT exception_unimp_inst(EXC_PARAM) {
  panic("Unimplemented instruction");
}
static void INTERRUPT exception_uninit_int_vector(EXC_PARAM) {
  panic("Uninitialized interrupt vector");
}
static void INTERRUPT exception_spurious_intr(EXC_PARAM) {
  panic("Spurious interrupt");
}
static void INTERRUPT autovector(EXC_PARAM) { panic("Autovector"); }
static void INTERRUPT trap(EXC_PARAM) { panic("Trap"); }
static void INTERRUPT user_interrupt(EXC_PARAM) { panic("User interrupt"); }

__attribute__((always_inline)) static inline void printstr(const char *str) {
  while (*str) {
    while ((MEM(DUART_SRB) & 0b00000100) == 0) {
    } // wait until TX is ready
    MEM(DUART_TBB) = *(str++);
  }
}
void panic(const char *err) {
  printstr("\r\nPANIC: ");
  printstr(err);
  printstr("\r\n");

  while (1) {
  }
}

void Irq_SetHandler(unsigned char exception_number,
                    void (*exception_handler)()) {
  ((uint32_t *)IRQ_TABLE)[exception_number] = (uint32_t)exception_handler;
}

void Irq_SetInterrupts(bool enabled) {
  if (enabled) {
    // Set minimum interrupt level to 0 in the status register
    __asm__ volatile("and.w #0xF8FF, %sr");
  } else {
    // Set minimum interrupt level to 7, i.e. only non-maskable interrupts
    // enabled
    __asm__ volatile("move.w #0x2700, %sr");
  }
}

void Irq_Init(void) {
  Irq_SetHandler(0, 0);
  Irq_SetHandler(1, 0);
  Irq_SetHandler(2, exception_bus_error);
  Irq_SetHandler(3, exception_addr_error);
  Irq_SetHandler(4, exception_illegal_inst);
  Irq_SetHandler(5, exception_div_zero);
  Irq_SetHandler(6, exception_chk);
  Irq_SetHandler(7, exception_trapv);
  Irq_SetHandler(8, exception_privilege_violation);
  Irq_SetHandler(10, exception_unimp_inst);
  Irq_SetHandler(11, exception_unimp_inst);
  Irq_SetHandler(15, exception_uninit_int_vector);
  Irq_SetHandler(24, exception_spurious_intr);

  for (int i = 25; i <= 255; i++) {
    if (i < 32) {
      // Autovector
      if (i == (EXCEPTION_AUTOVECTOR + IRQ_NUM_DUART)) {
        Irq_SetHandler((EXCEPTION_AUTOVECTOR + IRQ_NUM_DUART), &DUART_Irq);
      } else {
        Irq_SetHandler(i, &autovector);
      }
    } else if (i < 48) {
      // Trap block
      Irq_SetHandler(i, &trap);
    } else {
      // User interrupts
      Irq_SetHandler(i, &user_interrupt);
    }
  }

  // Set Vector Base Register to the newly filled IRQ table
  __asm__ volatile("movec %0, %%VBR" : : "r"(IRQ_TABLE) :);
}
