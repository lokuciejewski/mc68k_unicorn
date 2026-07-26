#include "ushell.h"
#include "ushell_cfg.h"

int ushell(void) {
  USH_PRINT("Hello from uShell!");
  static char cmd[256];
  uint8_t idx = 0;
  bool state = false;
  while (1) {
    char recv = USH_GETC();
    USH_PUTC(recv);
    cmd[idx] = recv;
    if (recv == '\n' || recv == '\r') {
      // TODO: execute command
      idx = 0;
    } else {
      idx++;
    }
    state = !state;
  }
  return 0;
}