#include "bootloader.h"
#include "hal/inc/gpio.h"
#include "hal/inc/irq.h"
#include "hal/inc/serial.h"
#include "hal/inc/spi.h"
#include "hal/inc/time.h"
#include "system/result_codes.h"
#include <stdio.h>
#include <string.h>

extern int tinylisp();

int main();

static volatile uint8_t USER_STACK[8192] = {0};

extern char __data_source[]; /* LMA – load address in flash */
extern char __data_start[];  /* VMA – run address in RAM */
extern char __data_size[];
extern char __bss_start[];
extern char __bss_size[];

inline static void __attribute__((always_inline)) enter_user_mode(void) {
  __asm__ volatile("move.l %0, %%usp\n\t" // Set user stack
                   "and.w #0xdfff, %%sr"  // Enter user mode
                   :
                   : "a"(USER_STACK + 8192 - 4)
                   : "memory", "cc");
}

void _start(void) {
  Irq_SetInterrupts(false);

  // copy .data from ROM to RAM
  char *src = __data_source;
  char *dst = __data_start;
  unsigned long n = (unsigned long)__data_size;
  while (n--) {
    *dst++ = *src++;
  }

  // zero .bss section
  dst = __bss_start;
  n = (unsigned long)__bss_size;
  while (n--) {
    *dst++ = 0;
  }

  Irq_Init();
  Serial_Settings_t settings_a = {.instance = Serial_A,
                                  .baudrate = Serial_Baudrate_19200,
                                  .bit_conf = Serial_8n,
                                  .stop_bits = Serial_StopBits1,
                                  .rx_rts = false,
                                  .tx_rts = false,
                                  .cts = false};
  Serial_Init(&settings_a);
  Serial_Settings_t settings_b = {.instance = Serial_B,
                                  .baudrate = Serial_Baudrate_300,
                                  .bit_conf = Serial_7e,
                                  .stop_bits = Serial_StopBits1,
                                  .rx_rts = true,
                                  .tx_rts = true,
                                  .cts = true};
  Serial_Init(&settings_b);
  GPIO_Init();
  GPIO_Led_Set(Led_1, false);
  GPIO_Led_Set(Led_2, false);
  GPIO_Led_Set(Led_3, false);
  Time_Init();
  SPI_Init();
  Irq_SetInterrupts(true);
  main();
}

int main() {
  enter_user_mode();
  printf("\r\nUnicorn SBC v1\r\n");
  return tinylisp();
}

static inline bool is_elf_valid(Elf32_Ehdr *hdr) {
  return memcmp(hdr->e_ident,
                "\x7f"
                "ELF",
                4) == 0 &&
         hdr->e_ident[4] == ELFCLASS32 && hdr->e_ident[5] == ELFDATA2MSB &&
         hdr->e_machine == EM_68K;
}

UnicornResult_e load_and_run_elf(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (f != nullptr) {
    Elf32_Ehdr hdr = {0};
    fread((void *)&hdr, sizeof(Elf32_Ehdr), 1, f);
    if (!is_elf_valid(&hdr)) {
      return UniRes_InvalidElf;
    }


  }
  return UniRes_NoSuchFile;
}