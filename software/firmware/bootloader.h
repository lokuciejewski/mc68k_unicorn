#pragma once
#include "result_codes.h"
#include <stdbool.h>
#include <stdint.h>

#define EI_NIDENT 16
#define ELFCLASS32 1
#define ELFDATA2MSB 2 // Big Endian (m68k)
#define EM_68K 4      // Motorola 68000 series

#define PT_LOAD 1 // Loadable segment

#define ET_EXEC 2 // Executable file

typedef struct {
  unsigned char e_ident[EI_NIDENT]; // Magic + class
  uint16_t e_type;      // should be 2 (ET_EXEC, fixed address executable)
  uint16_t e_machine;   // MC68K
  uint32_t e_version;   // always 1?
  uint32_t e_entry;     // Entry point virtual address
  uint32_t e_phoff;     // Program header table offset
  uint32_t e_shoff;     // Ignored
  uint32_t e_flags;     // Ignored but indicates the CPU model
  uint16_t e_ehsize;    // Executable header size
  uint16_t e_phentsize; // Program header size
  uint16_t e_phnum;     // Number of program headers
  uint16_t e_shentsize; // Ignored
  uint16_t e_shnum;     // Ignored
  uint16_t e_shstrndx;  // Ignored
} Elf32_Ehdr;

typedef struct {
  uint32_t p_type;   // Segment type (e.g. PT_LOAD)
  uint32_t p_offset; // Segment file offset
  uint32_t p_vaddr;  // Segment virtual address in RAM
  uint32_t p_paddr;  // Segment physical address
  uint32_t p_filesz; // Segment size in file
  uint32_t p_memsz;  // Segment size in memory (>= p_filesz)
  uint32_t p_flags;  // Ignored, holds info about whether the segment is
                     // eXecutable, Readable or Writable
  uint32_t p_align;  // Should be a multiple of 0x2000 but does not matter if
                     // there is no page mapping nor arbitrary address load
} Elf32_Phdr;

typedef int (*app_entry_t)(void);

UnicornResult_e load_elf(const char *filename, uintptr_t *out_entry_point);