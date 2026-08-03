#include "../inc/primitives.h"
#include "../../hal/inc/memory.h"
#include "../inc/memory.h"
#include "../inc/minilisp_int.h"
#include "../inc/parser.h"
#include <stdio.h>
#include <stdlib.h>

static void add_primitive(void *root, Obj **env, char *name, Primitive *fn) {
  DEFINE2(sym, prim);
  *sym = intern(root, name);
  *prim = make_primitive(root, fn);
  add_variable(root, env, sym, prim);
}

void define_constants(void *root, Obj **env) {
  DEFINE1(sym);
  *sym = intern(root, "t");
  add_variable(root, env, sym, &True);
}

//======================================================================
// Primitive functions and special forms
//======================================================================

// 'expr
static Obj *prim_quote(void *root, Obj **env, Obj **list) {
  if (length(*list) != 1) {
    error("Malformed quote");
    return Nil;
  }
  return (*list)->car;
}

// (cons expr expr)
static Obj *prim_cons(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2) {
    error("Malformed cons");
    return Nil;
  }
  Obj *cell = eval_list(root, env, list);
  cell->cdr = cell->cdr->car;
  return cell;
}

// (car <cell>)
static Obj *prim_car(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  if (args->car->type != TCELL || args->cdr != Nil) {
    error("Malformed car");
    return Nil;
  }
  return args->car->car;
}

// (cdr <cell>)
static Obj *prim_cdr(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  if (args->car->type != TCELL || args->cdr != Nil) {
    error("Malformed cdr");
    return Nil;
  }
  return args->car->cdr;
}

// (setq <symbol> expr)
static Obj *prim_setq(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2 || (*list)->car->type != TSYMBOL) {
    error("Malformed setq");
    return Nil;
  }
  DEFINE2(bind, value);
  *bind = find(env, (*list)->car);
  if (!*bind) {
    error("Unbound variable %s", (*list)->car->name);
    return Nil;
  }
  *value = (*list)->cdr->car;
  *value = eval(root, env, value);
  (*bind)->cdr = *value;
  return *value;
}

// (setcar <cell> expr)
static Obj *prim_setcar(void *root, Obj **env, Obj **list) {
  DEFINE1(args);
  *args = eval_list(root, env, list);
  if (length(*args) != 2 || (*args)->car->type != TCELL) {
    error("Malformed setcar");
    return Nil;
  }
  (*args)->car->car = (*args)->cdr->car;
  return (*args)->car;
}

// (while cond expr ...)
static Obj *prim_while(void *root, Obj **env, Obj **list) {
  if (length(*list) < 2) {
    error("Malformed while");
    return Nil;
  }
  DEFINE2(cond, exprs);
  *cond = (*list)->car;
  while (eval(root, env, cond) != Nil) {
    *exprs = (*list)->cdr;
    eval_list(root, env, exprs);
  }
  return Nil;
}

// (gensym)
static Obj *prim_gensym(void *root, Obj **env, Obj **list) {
  static int count = 0;
  char buf[10];
  snprintf(buf, sizeof(buf), "G__%d", count++);
  return make_symbol(root, buf);
}

// (+ <integer> ...)
static Obj *prim_plus(void *root, Obj **env, Obj **list) {
  int sum = 0;
  for (Obj *args = eval_list(root, env, list); args != Nil; args = args->cdr) {
    if (args->car->type != TINT) {
      error("+ takes only numbers");
      return Nil;
    }
    sum += args->car->value;
  }
  return make_int(root, sum);
}

// (- <integer> ...)
static Obj *prim_minus(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  for (Obj *p = args; p != Nil; p = p->cdr)
    if (p->car->type != TINT) {
      error("- takes only numbers");
      return Nil;
    }
  if (args->cdr == Nil)
    return make_int(root, -args->car->value);
  int r = args->car->value;
  for (Obj *p = args->cdr; p != Nil; p = p->cdr)
    r -= p->car->value;
  return make_int(root, r);
}

// (* <integer> ...)
static Obj *prim_mult(void *root, Obj **env, Obj **list) {
  int mult = 1;
  for (Obj *args = eval_list(root, env, list); args != Nil; args = args->cdr) {
    if (args->car->type != TINT) {
      error("* takes only numbers");
      return Nil;
    }
    mult *= args->car->value;
  }
  return make_int(root, mult);
}

// (<< <integer> <integer>)
static Obj *prim_shl(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  if (length(args) != 2) {
    error("malformed <<");
    return Nil;
  }
  Obj *x = args->car;
  Obj *y = args->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error("<< takes only numbers");
    return Nil;
  }
  return make_int(root, x->value << y->value);
}

// (<< <integer> <integer>)
static Obj *prim_shr(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  if (length(args) != 2) {
    error("malformed >>");
    return Nil;
  }
  Obj *x = args->car;
  Obj *y = args->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error(">> takes only numbers");
    return Nil;
  }
  return make_int(root, x->value >> y->value);
}

// (< <integer> <integer>)
static Obj *prim_lt(void *root, Obj **env, Obj **list) {
  Obj *args = eval_list(root, env, list);
  if (length(args) != 2) {
    error("malformed <");
    return Nil;
  }
  Obj *x = args->car;
  Obj *y = args->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error("< takes only numbers");
    return Nil;
  }
  return x->value < y->value ? True : Nil;
}

Obj *handle_function(void *root, Obj **env, Obj **list, int type) {
  if ((*list)->type != TCELL || !is_list((*list)->car) ||
      (*list)->cdr->type != TCELL) {
    error("Malformed lambda");
    return Nil;
  }
  Obj *p = (*list)->car;
  for (; p->type == TCELL; p = p->cdr)
    if (p->car->type != TSYMBOL) {
      error("Parameter must be a symbol");
      return Nil;
    }
  if (p != Nil && p->type != TSYMBOL) {
    error("Parameter must be a symbol");
    return Nil;
  }
  DEFINE2(params, body);
  *params = (*list)->car;
  *body = (*list)->cdr;
  return make_function(root, env, type, params, body);
}

// (lambda (<symbol> ...) expr ...)
static Obj *prim_lambda(void *root, Obj **env, Obj **list) {
  return handle_function(root, env, list, TFUNCTION);
}

static Obj *handle_defun(void *root, Obj **env, Obj **list, int type) {
  if ((*list)->car->type != TSYMBOL || (*list)->cdr->type != TCELL) {
    error("Malformed defun");
    return Nil;
  }
  DEFINE3(fn, sym, rest);
  *sym = (*list)->car;
  *rest = (*list)->cdr;
  *fn = handle_function(root, env, rest, type);
  add_variable(root, env, sym, fn);
  return *fn;
}

// (defun <symbol> (<symbol> ...) expr ...)
static Obj *prim_defun(void *root, Obj **env, Obj **list) {
  return handle_defun(root, env, list, TFUNCTION);
}

// (define <symbol> expr)
static Obj *prim_define(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2 || (*list)->car->type != TSYMBOL) {
    error("Malformed define");
    return Nil;
  }
  DEFINE2(sym, value);
  *sym = (*list)->car;
  *value = (*list)->cdr->car;
  *value = eval(root, env, value);
  add_variable(root, env, sym, value);
  return *value;
}

// (defmacro <symbol> (<symbol> ...) expr ...)
static Obj *prim_defmacro(void *root, Obj **env, Obj **list) {
  return handle_defun(root, env, list, TMACRO);
}

// (macroexpand expr)
static Obj *prim_macroexpand(void *root, Obj **env, Obj **list) {
  if (length(*list) != 1) {
    error("Malformed macroexpand");
    return Nil;
  }
  DEFINE1(body);
  *body = (*list)->car;
  return macroexpand(root, env, body);
}

// (println expr)
static Obj *prim_println(void *root, Obj **env, Obj **list) {
  DEFINE1(tmp);
  *tmp = (*list)->car;
  print(eval(root, env, tmp));
  printf("\n");
  return Nil;
}

// (if expr expr expr ...)
static Obj *prim_if(void *root, Obj **env, Obj **list) {
  if (length(*list) < 2) {
    error("Malformed if");
    return Nil;
  }
  DEFINE3(cond, then, els);
  *cond = (*list)->car;
  *cond = eval(root, env, cond);
  if (*cond != Nil) {
    *then = (*list)->cdr->car;
    return eval(root, env, then);
  }
  *els = (*list)->cdr->cdr;
  return *els == Nil ? Nil : progn(root, env, els);
}

// (= <integer> <integer>)
static Obj *prim_num_eq(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2) {
    error("Malformed =");
    return Nil;
  }
  Obj *values = eval_list(root, env, list);
  Obj *x = values->car;
  Obj *y = values->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error("= only takes numbers");
    return Nil;
  }
  return x->value == y->value ? True : Nil;
}

// (eq expr expr)
static Obj *prim_eq(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2) {
    error("Malformed eq");
    return Nil;
  }
  Obj *values = eval_list(root, env, list);
  return values->car == values->cdr->car ? True : Nil;
}

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

// (rmem <integer> <integer>)
static Obj *prim_rmem(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2) {
    error("Malformed rmem");
    return Nil;
  }
  Obj *values = eval_list(root, env, list);
  Obj *x = values->car;
  Obj *y = values->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error("rmem only takes numbers (addr, len)");
    return Nil;
  }
  hexdump(x->value, y->value);
  return Nil;
}

// (rmem <integer> <integer>)
static Obj *prim_wmem(void *root, Obj **env, Obj **list) {
  if (length(*list) != 2) {
    error("Malformed wmem");
    return Nil;
  }
  Obj *values = eval_list(root, env, list);
  Obj *x = values->car;
  Obj *y = values->cdr->car;
  if (x->type != TINT || y->type != TINT) {
    error("wmem only takes numbers (addr, values...)");
    return Nil;
  }
  DEFINE1(ret);
  (*ret)->type = TINT;
  if (y->value < UINT8_MAX) {
    *ret = make_int(root, MEM(x->value));
    MEM(x->value) = (uint8_t)(y->value);
  } else if (y->value < UINT16_MAX) {
    if (x->value % 2 != 0) {
      error("Malformed wmem");
      return Nil;
    } else {
      *ret = make_int(root, MEM16(x->value));
      MEM16(x->value) = (uint16_t)(y->value);
    }
  } else {
    error("Malformed wmem"); // TODO: change basic type to uint32_t and add sign
                             // bool to Obj
    // if (x->value % 4 != 0) {
    //   error("Malformed wmem");
    //   return Nil;
    // } else {
    //   *ret = make_int(root, MEM32((uint32_t)(x->value)));
    //   MEM32((uint32_t)(x->value)) = (uint32_t)(y->value);
    // }
  }
  return *ret;
}

// (println expr)
static Obj *prim_hex(void *root, Obj **env, Obj **list) {
  DEFINE3(tmp, evald, res_int);
  *tmp = (*list)->car;
  char *end;
  *evald = eval(root, env, tmp);
  unsigned long result = strtoul((*evald)->name, &end, 16);
  *res_int = make_int(root, result);
  if (end == (*evald)->name || *end != '\0') {
    // invalid or out of 16-bit range
    error("Malformed hex");
    return Nil;
  }
  return *res_int;
}

void define_primitives(void *root, Obj **env) {
  add_primitive(root, env, "quote", prim_quote);
  add_primitive(root, env, "cons", prim_cons);
  add_primitive(root, env, "car", prim_car);
  add_primitive(root, env, "cdr", prim_cdr);
  add_primitive(root, env, "setq", prim_setq);
  add_primitive(root, env, "setcar", prim_setcar);
  add_primitive(root, env, "while", prim_while);
  add_primitive(root, env, "gensym", prim_gensym);
  add_primitive(root, env, "+", prim_plus);
  add_primitive(root, env, "-", prim_minus);
  add_primitive(root, env, "*", prim_mult);
  add_primitive(root, env, "<<", prim_shl);
  add_primitive(root, env, ">>", prim_shr);
  add_primitive(root, env, "<", prim_lt);
  add_primitive(root, env, "define", prim_define);
  add_primitive(root, env, "defun", prim_defun);
  add_primitive(root, env, "defmacro", prim_defmacro);
  add_primitive(root, env, "macroexpand", prim_macroexpand);
  add_primitive(root, env, "lambda", prim_lambda);
  add_primitive(root, env, "if", prim_if);
  add_primitive(root, env, "=", prim_num_eq);
  add_primitive(root, env, "eq", prim_eq);
  add_primitive(root, env, "println", prim_println);
  add_primitive(root, env, "rmem", prim_rmem);
  add_primitive(root, env, "wmem", prim_wmem);
  add_primitive(root, env, "hex", prim_hex);
}