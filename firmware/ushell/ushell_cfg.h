#pragma once

#include "../hal/inc/serial.h"

#define USHELL_SERIAL Serial_A

#define USH_PRINT(str) Serial_PrintStr(USHELL_SERIAL, str "\r\n")
#define USH_PUTC(c) Serial_Putc(USHELL_SERIAL, c)
#define USH_GETC() Serial_Getc(USHELL_SERIAL)