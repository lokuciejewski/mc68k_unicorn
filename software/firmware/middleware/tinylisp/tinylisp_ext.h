// Unicorn-specific tinylisp extensions
// DO NOT INCLUDE IN FILES OTHER THAN `tinylisp.c`
// Include this file above `prim` definition and add `UNICORN_EXT` to the `prim`
// array to use the functions in this file
#pragma once

#include "../../hal/inc/memory.h"
#include "../../hal/inc/time.h"
#include "../fatfs/api/ff.h"
#include "bootloader.h"
#include "result_codes.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef I
#define I unsigned
#endif
#ifndef L
#define L double
#endif
#ifndef A
#define A (char *)cell
#endif
#ifndef N
#define N 1024
#endif

extern L cell[N], nil, tru, env;
extern I ord(L x);

extern L evarg(L *t, L *e, I *a);
extern void gc(L x);
extern L err(I i);
extern I isarg(L *t, L *e, I *a, L *x);
extern L cons(L x, L y);
extern L num(L n);

static void hexdump(uint32_t addr, uint32_t len) {
  printf("\r\n");
  char ascii[17];
  ascii[16] = '\0';

  for (uint32_t i = 0; i < len; i++) {
    if (i % 16 == 0) {
      if (i != 0)
        printf(" |%s|\n", ascii);
      printf("0x%08lx: ", addr + i);
    }

    uint8_t b = MEM(addr + i);
    printf("%02x ", b);

    ascii[i % 16] = (b >= 32 && b < 127) ? b : '.';

    if (i + 1 == len) {
      for (uint32_t j = (i % 16) + 1; j < 16; j++) {
        printf("   ");
        ascii[j] = ' ';
      }
      printf(" |%s|\n", ascii);
    }
  }
}

L f_rmem(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = evarg(&t, e, &a);
  gc(x);
  gc(y);
  hexdump(x, y);
  return nil;
}

L f_wmem(L t, L *e) {
  I a = 0;
  L x, n = evarg(&t, e, &a);
  if (n < 0x10000) {
    err(1);
    return nil;
  }
  for (uint16_t i = 0; isarg(&t, e, &a, &x); i++) {
    gc(n);
    gc(x);
    MEM(n + i) = x;
  }
  return tru;
}

L f_lsh(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = evarg(&t, e, &a);
  gc(x);
  gc(y);
  return num((I)x << (I)y);
}

L f_delay_ms(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  gc(x);
  delay_ms(x);
  return nil;
}

static FATFS sd_card;
L f_fs_mount([[maybe_unused]] L t, [[maybe_unused]] L *e) {
  return num(f_mount(&sd_card, "/", 0));
}

L f_fs_open(L t, L *e) {
  I a = 0;
  L path = evarg(&t, e, &a);
  L open_flags = evarg(&t, e, &a);
  int fd = open(A + ord(path), open_flags);
  gc(path);
  gc(open_flags);
  return num(fd);
}

L f_fs_write(L t, L *e) {
  I a = 0;
  L fd = evarg(&t, e, &a);
  L line = evarg(&t, e, &a);
  UINT result = write(fd, A + ord(line), strlen(A + ord(line)));
  gc(fd);
  gc(line);
  return num(result);
}

L f_fs_lseek(L t, L *e) {
  I a = 0;
  L fd = evarg(&t, e, &a), offset = evarg(&t, e, &a), whence = evarg(&t, e, &a);
  gc(fd);
  gc(offset);
  gc(whence);
  return num(lseek(fd, offset, whence));
}

L f_fs_read(L t, L *e) {
  I a = 0;
  L fd = evarg(&t, e, &a);
  gc(fd);
  char buf[128] = {0};
  UINT br = read(fd, buf, sizeof(buf));
  printf("%s\r\n", buf);
  return num(br);
}

L f_fs_close(L t, L *e) {
  I a = 0;
  L fd = evarg(&t, e, &a);
  gc(fd);
  return num(close(fd));
}

L f_bootloader_exec(L t, L *e) {
  I a = 0;
  L path = evarg(&t, e, &a);
  uintptr_t entry_addr = 0;
  int return_val = -1;
  UnicornResult_e load_res = load_elf(A + ord(path), &entry_addr);
  gc(path);
  if (load_res == UniRes_Ok) {
    return_val = ((app_entry_t)entry_addr)();
    return num(return_val);
  } else {
    return num(load_res);
  }
}

// clang-format off
#define UNICORN_EXT {"<<", f_lsh, 0},               \
                    {"rmem", f_rmem, 0},            \
                    {"wmem", f_wmem, 0},            \
                    {"exec", f_bootloader_exec, 0}, \
                    {"mount", f_fs_mount, 0},       \
                    {"open", f_fs_open, 0},         \
                    {"write", f_fs_write, 0},       \
                    {"lseek", f_fs_lseek, 0},       \
                    {"read", f_fs_read, 0},         \
                    {"close", f_fs_close, 0},       \
                    {"q", f_quote, 0},              \
                    {"delay", f_delay_ms, 0}
// clang-format on