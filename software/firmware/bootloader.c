#include "bootloader.h"
#include "hal/inc/gpio.h"
#include "hal/inc/irq.h"
#include "hal/inc/serial.h"
#include "hal/inc/spi.h"
#include "hal/inc/time.h"
#include "system/result_codes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int tinylisp();
extern uint8_t __ram[];
extern uint8_t __flash[];
extern uint8_t __flash_size[];

int main();

// static volatile uint8_t USER_STACK[4096] = {0};

extern char __data_source[]; /* LMA – load address in flash */
extern char __data_start[];  /* VMA – run address in RAM */
extern char __data_size[];
extern char __bss_start[];
extern char __bss_size[];

// inline static void __attribute__((always_inline)) enter_user_mode(void) {
//   __asm__ volatile("move.l %0, %%usp\n\t" // Set user stack
//                    "and.w #0xdfff, %%sr"  // Enter user mode
//                    :
//                    : "a"(USER_STACK + 4096 - 4)
//                    : "memory", "cc");
// }

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
  // enter_user_mode();
  printf("\r\nUnicorn SBC v1\r\n");
  return tinylisp();
}

static inline bool is_ehdr_valid(Elf32_Ehdr *hdr) {
  return memcmp(hdr->e_ident,
                "\x7f"
                "ELF",
                4) == 0 &&
         hdr->e_ident[4] == ELFCLASS32 && hdr->e_ident[5] == ELFDATA2MSB &&
         hdr->e_machine == EM_68K && hdr->e_ehsize == sizeof(Elf32_Ehdr) &&
         hdr->e_phentsize == sizeof(Elf32_Phdr) && hdr->e_type == ET_EXEC;
}

static inline bool is_phdr_valid(Elf32_Phdr *phdr) {
  return (phdr->p_memsz >= phdr->p_filesz &&
          phdr->p_vaddr + phdr->p_memsz <= (uintptr_t)__ram &&
          phdr->p_vaddr >= (uintptr_t)__flash + (uintptr_t)__flash_size);
}

UnicornResult_e load_elf(const char *filename, uintptr_t *out_entry_point) {
  FILE *f = fopen(filename, "r");
  size_t read = 0;
  if (f != nullptr) {
    Elf32_Ehdr ehdr = {0};
    read = fread((void *)&ehdr, sizeof(Elf32_Ehdr), 1, f);
    if (1 == read) {
      if (!is_ehdr_valid(&ehdr)) {
        printf("Elf header is invalid\r\n");
        return UniRes_InvalidHeader;
      }
      // Elf is valid, load all program headers
      Elf32_Phdr phdr = {0};
      for (uint16_t ph = 0; ph < ehdr.e_phnum; ph++) {
        (void)fseek(f, ehdr.e_phoff + (ph * sizeof(Elf32_Phdr)), SEEK_SET);
        read = fread(&phdr, sizeof(Elf32_Phdr), 1, f);
        if (1 == read) {
          switch (phdr.p_type) {
          case PT_LOAD: {
            if (!is_phdr_valid(&phdr)) {
              fclose(f);
              printf("Program header %d is invalid\r\n", ph);
              return UniRes_InvalidHeader;
            }
            // PHDR valid, load it
            (void)fseek(f, phdr.p_offset, SEEK_SET);
            read = fread((void *)phdr.p_vaddr, 1, phdr.p_filesz, f);
            if (phdr.p_filesz == read) {
              memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0,
                     (phdr.p_memsz -
                      phdr.p_filesz)); // Zero out the rest of the segment
            } else {
              fclose(f);
              printf("Read of %d header contents failed: %lu\r\n", ph, read);
              return UniRes_GeneralError;
            }
          } break;
          default:
            // Skip other header types for now
            break;
          }
        } else {
          fclose(f);
          printf("Read of header %d failed: %lu\r\n", ph, read);
          return UniRes_GeneralError;
        }
      }

      *out_entry_point = ehdr.e_entry;
      fclose(f);
      return UniRes_Ok;
    } else {
      fclose(f);
      printf("Read of elf header failed: %lu\r\n", read);
      return UniRes_GeneralError;
    }
  }
  return UniRes_NotFound;
}