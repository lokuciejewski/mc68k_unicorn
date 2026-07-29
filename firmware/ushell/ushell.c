#include "ushell.h"
#include "ushell_cfg.h"

#include <stdio.h>
#include <string.h>

int ushell(void) {
  USH_PRINT("Hello from uShell!");
  static char cmd[256];
  uint8_t idx = 0;
  bool state = false;
  while (1) {
    char recv = USH_GETC();
    if (recv == '\n' || recv == '\r') {
      USH_PRINT("");
      if (strcmp(cmd, "test") == 0) {
        USH_PRINT("!");
      } else {
        USH_PRINT("?");
      }
      idx = 0;
    } else {
      USH_PUTC(recv);
      cmd[idx] = recv;
      idx++;
    }
    state = !state;
  }
  return 0;
}