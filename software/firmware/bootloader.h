#pragma once
#include "result_codes.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define EI_NIDENT 16
#define ELFCLASS32 1
#define ELFDATA2MSB 2 /* Big Endian (m68k) */
#define EM_68K 4      /* Motorola 68000 series */
#define PT_LOAD 1     /* Loadable segment */

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry; /* Entry point virtual address */
  uint32_t e_phoff; /* Program header table offset */
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum; /* Number of program headers */
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  uint32_t p_type;   /* Segment type (e.g. PT_LOAD) */
  uint32_t p_offset; /* Segment file offset */
  uint32_t p_vaddr;  /* Segment virtual address in RAM */
  uint32_t p_paddr;  /* Segment physical address */
  uint32_t p_filesz; /* Segment size in file */
  uint32_t p_memsz;  /* Segment size in memory (>= p_filesz) */
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

typedef void (*app_entry_t)(void);

UnicornResult_e load_and_run_elf(const char *filename);