#include "bootloader.h"
#include "ff.h"
#include "hal/inc/gpio.h"
#include "hal/inc/irq.h"
#include "hal/inc/serial.h"
#include "hal/inc/spi.h"
#include "hal/inc/time.h"
#include "system/result_codes.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BOOT_ORDER_FILE "/bootord"
#define DEFAULT_STDIN_SERIAL Serial_A
#define BOOT_TIMEOUT_S 3U

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

static FATFS sd_card;
int main() {
  // enter_user_mode();
  printf("\r\nUnicorn SBC v1\r\n");
  f_mount(&sd_card, "/", 0);
  int res = try_booting_from_bootord();
  printf("BOOT: %d\r\n", res);
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
    printf("LOAD from %s\r\n", filename);
    Elf32_Ehdr ehdr = {0};
    read = fread((void *)&ehdr, sizeof(Elf32_Ehdr), 1, f);
    if (1 == read) {
      if (!is_ehdr_valid(&ehdr)) {
        printf("CHECK ehdr FAIL\r\n");
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
              printf("CHECK phdr%d FAIL\r\n", ph);
              return UniRes_InvalidHeader;
            }
            printf("LOAD phdr%d [%lu bytes] @ 0x%06lx\r\n", ph, phdr.p_filesz,
                   phdr.p_vaddr);
            // PHDR valid, load it
            (void)fseek(f, phdr.p_offset, SEEK_SET);
            read = fread((void *)phdr.p_vaddr, 1, phdr.p_filesz, f);
            if (phdr.p_filesz == read) {
              size_t zero_addr = phdr.p_vaddr + phdr.p_filesz;
              size_t zero_count = phdr.p_memsz - phdr.p_filesz;
              printf("ZERO [%lu bytes] @ 0x%06lx\r\n", zero_count, zero_addr);
              memset((void *)(zero_addr), 0,
                     (zero_count)); // Zero out the rest of the segment
            } else {
              fclose(f);
              printf("READ phdr%d FAIL: %lu\r\n", ph, read);
              return UniRes_GeneralError;
            }
          } break;
          default:
            // Skip other header types for now
            break;
          }
        } else {
          fclose(f);
          printf("READ phdr%d FAIL: %lu\r\n", ph, read);
          return UniRes_GeneralError;
        }
      }

      *out_entry_point = ehdr.e_entry;
      fclose(f);
      printf("LOAD OK\r\n");
      return UniRes_Ok;
    } else {
      fclose(f);
      printf("READ ehdr FAIL: %lu\r\n", read);
      return UniRes_GeneralError;
    }
  }
  return UniRes_NotFound;
}

UnicornResult_e try_booting_from_bootord(void) {
  FILE *f = fopen(BOOT_ORDER_FILE, "r");
  if (f != nullptr) {
    char filepath[128] = {0};
    uint8_t read = fread(filepath, 1, 128, f);
    fclose(f);
    if (read > 0) {
      for (uint8_t i = 0; i < read; i++) {
        if (filepath[i] == '\r' || filepath[i] == '\n') {
          filepath[i] = '\0';
        }
      }
      uintptr_t entry_addr = 0;
      UnicornResult_e load_res = load_elf(filepath, &entry_addr);
      if (load_res == UniRes_Ok) {
        (void)((app_entry_t)entry_addr)();
        return UniRes_Ok;
      }
    } else {
      return UniRes_InvalidData;
    }
  }
  return UniRes_NotFound;
}