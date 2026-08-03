#include "../inc/minilisp_int.h"

#include <stdio.h>
#include <stdarg.h>

 void error(char *fmt, ...) {
  printf("\r\n");
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}