/// Based on: https://github.com/rui314/minilisp

// This software is in the public domain.

#include "inc/memory.h"
#include "inc/minilisp_int.h"
#include "inc/parser.h"
#include "inc/primitives.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//======================================================================
// Lisp objects
//======================================================================

// Constants
Obj *True = &(Obj){TTRUE};
Obj *Nil = &(Obj){TNIL};
Obj *Dot = &(Obj){TDOT};
Obj *Cparen = &(Obj){TCPAREN};

Obj *Symbols;

//======================================================================
// Entry point
//======================================================================

// Returns true if the environment variable is defined and not the empty string.
static bool getEnvFlag(char *name) {
  char *val = getenv(name);
  return val && val[0];
}

int minilisp(int argc, char **argv) {
  init_memory();

  // Constants and primitives
  Symbols = Nil;
  void *root = NULL;
  DEFINE2(env, expr);
  *env = make_env(root, &Nil, &Nil);
  define_constants(root, env);
  define_primitives(root, env);

  // The main loop
  for (;;) {
    *expr = read_expr(root);
    if (!*expr)
      return 0;
    if (*expr == Cparen) {
      error("Stray close parenthesis");
    } else if (*expr == Dot) {
      error("Stray dot");
    } else {
      print(eval(root, env, expr));
    }
    printf("\n");
  }
}