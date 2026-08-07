#include <stdio.h>

#include "hal/inc/serial.h"

static int uart_putchar(char c, [[maybe_unused]] FILE *stream) {
  Serial_Putc(Serial_A, c);
  return c;
}

static int uart_getchar([[maybe_unused]] FILE *stream) {
  return Serial_Getc(Serial_A);
}

static FILE __stdio =
    FDEV_SETUP_STREAM(uart_putchar, uart_getchar, NULL, _FDEV_SETUP_RW);

FILE *const stdin __attribute__((used)) = &__stdio;
FILE *const stdout __attribute__((used)) = &__stdio;
FILE *const stderr __attribute__((used)) = &__stdio;
FILE *const __iob[3] = {&__stdio, &__stdio, &__stdio};

int __attribute((noreturn)) _exit(int status) {
  printf("exit status %d\r\n", status);
  __asm__ volatile(
      "move.w #0x2700, %%sr\n\t"     // supervisor mode, interrupts disabled
      "movea.l 0x00000000, %%sp\n\t" // load initial SSP from reset vector
      "movea.l 0x00000004, %%a0\n\t" // load initial PC
      "jmp (%%a0)"                   // jump to reset entry point
      :
      :
      : "a0", "cc", "memory");

  while(1) {}
}