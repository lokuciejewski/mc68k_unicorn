#include "../inc/parser.h"
#include "../inc/memory.h"
#include "../inc/minilisp_int.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//======================================================================
// Parser
//
// This is a hand-written recursive-descendent parser.
//======================================================================

#define SYMBOL_MAX_LEN 200
const char symbol_chars[] = "~!@#$%^&*-_=+:/?<>";

static int peek(void) {
  int c = getchar();
  ungetc(c, stdin);
  return c;
}

// Skips the input until newline is found. Newline is one of \r, \r\n or \n.
static void skip_line(void) {
  for (;;) {
    int c = getchar();
    if (c == EOF || c == '\n')
      return;
    if (c == '\r') {
      if (peek() == '\n')
        getchar();
      return;
    }
  }
}

static Obj *reverse(Obj *p) {
  Obj *ret = Nil;
  while (p != Nil) {
    Obj *head = p;
    p = p->cdr;
    head->cdr = ret;
    ret = head;
  }
  return ret;
}

// Reads a list. Note that '(' has already been read.
static Obj *read_list(void *root) {
  DEFINE3(obj, head, last);
  *head = Nil;
  for (;;) {
    *obj = read_expr(root);
    if (!*obj) {
      error("Unclosed parenthesis");
      return Nil;
    }
    if (*obj == Cparen)
      return reverse(*head);
    if (*obj == Dot) {
      *last = read_expr(root);
      if (read_expr(root) != Cparen) {
        error("Closed parenthesis expected after dot");
        return Nil;
      }
      Obj *ret = reverse(*head);
      (*head)->cdr = *last;
      return ret;
    }
    *head = cons(root, obj, head);
  }
}

Obj *intern(void *root, char *name) {
  for (Obj *p = Symbols; p != Nil; p = p->cdr)
    if (strcmp(name, p->car->name) == 0)
      return p->car;
  DEFINE1(sym);
  *sym = make_symbol(root, name);
  Symbols = cons(root, sym, &Symbols);
  return *sym;
}

// Reader marcro ' (single quote). It reads an expression and returns (quote
// <expr>).
static Obj *read_quote(void *root) {
  DEFINE2(sym, tmp);
  *sym = intern(root, "quote");
  *tmp = read_expr(root);
  *tmp = cons(root, tmp, &Nil);
  *tmp = cons(root, sym, tmp);
  return *tmp;
}

// Reader marcro "..." (double quote). It reads an expression and returns (quote
// <expr>).
static Obj *read_string(void *root) {
  DEFINE1(str);
  char buf[256] = {0};
  for (uint8_t i = 0; i < 255; i++) {
    int c = getchar();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
      continue;
    if (c == EOF)
      return NULL;
    if (c == '\"')
      break;

    buf[i] = c;
  }
  *str = make_string(root, buf);
  return *str;
}

static int read_number(int val) {
  while (isdigit(peek()))
    val = val * 10 + (getchar() - '0');
  return val;
}

static Obj *read_symbol(void *root, char c) {
  char buf[SYMBOL_MAX_LEN + 1];
  buf[0] = c;
  int len = 1;
  while (isalnum(peek()) || strchr(symbol_chars, peek())) {
    if (SYMBOL_MAX_LEN <= len) {
      error("Symbol name too long");
      return Nil;
    }
    buf[len++] = getchar();
  }
  buf[len] = '\0';
  return intern(root, buf);
}

Obj *read_expr(void *root) {
  for (;;) {
    int c = getchar();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
      continue;
    if (c == EOF)
      return NULL;
    if (c == ';') {
      skip_line();
      continue;
    }
    if (c == '(')
      return read_list(root);
    if (c == ')')
      return Cparen;
    if (c == '.')
      return Dot;
    if (c == '\'')
      return read_quote(root);
    if (c == '\"')
      return read_string(root);
    if (isdigit(c))
      return make_int(root, read_number(c - '0'));
    if (c == '-' && isdigit(peek()))
      return make_int(root, -read_number(0));
    if (isalpha(c) || strchr(symbol_chars, c)) {
      return read_symbol(root, c);
    }
    error("Don't know how to handle %c", c);
    return Nil;
  }
}

void print(Obj *obj) {
  printf("\r\n");
  switch (obj->type) {
  case TCELL:
    printf("(");
    for (;;) {
      print(obj->car);
      if (obj->cdr == Nil)
        break;
      if (obj->cdr->type != TCELL) {
        printf(" . ");
        print(obj->cdr);
        break;
      }
      printf(" ");
      obj = obj->cdr;
    }
    printf(")");
    return;

#define CASE(type, ...)                                                        \
  case type:                                                                   \
    printf(__VA_ARGS__);                                                       \
    return
    CASE(TINT, "%ld", obj->value);
    CASE(TSYMBOL, "%s", obj->name);
    CASE(TSTR, "%s", obj->name);
    CASE(TPRIMITIVE, "<primitive>");
    CASE(TFUNCTION, "<function>");
    CASE(TMACRO, "<macro>");
    CASE(TMOVED, "<moved>");
    CASE(TTRUE, "t");
    CASE(TNIL, "()");
#undef CASE
  default:
    error("Bug: print: Unknown tag type: %d", obj->type);
  }
}

int length(Obj *list) {
  int len = 0;
  for (; list->type == TCELL; list = list->cdr)
    len++;
  return list == Nil ? len : -1;
}

//======================================================================
// Evaluator
//======================================================================

void add_variable(void *root, Obj **env, Obj **sym, Obj **val) {
  DEFINE2(vars, tmp);
  *vars = (*env)->vars;
  *tmp = acons(root, sym, val, vars);
  (*env)->vars = *tmp;
}

// Returns a newly created environment frame.
static Obj *push_env(void *root, Obj **env, Obj **vars, Obj **vals) {
  DEFINE3(map, sym, val);
  *map = Nil;
  for (; (*vars)->type == TCELL; *vars = (*vars)->cdr, *vals = (*vals)->cdr) {
    if ((*vals)->type != TCELL) {
      error("Cannot apply function: number of argument does not match");
      return Nil;
    }
    *sym = (*vars)->car;
    *val = (*vals)->car;
    *map = acons(root, sym, val, map);
  }
  if (*vars != Nil)
    *map = acons(root, vars, vals, map);
  return make_env(root, map, env);
}

Obj *progn(void *root, Obj **env, Obj **list) {
  DEFINE2(lp, r);
  for (*lp = *list; *lp != Nil; *lp = (*lp)->cdr) {
    *r = (*lp)->car;
    *r = eval(root, env, r);
  }
  return *r;
}

// Evaluates all the list elements and returns their return values as a new
// list.
Obj *eval_list(void *root, Obj **env, Obj **list) {
  DEFINE4(head, lp, expr, result);
  *head = Nil;
  for (lp = list; *lp != Nil; *lp = (*lp)->cdr) {
    *expr = (*lp)->car;
    *result = eval(root, env, expr);
    *head = cons(root, result, head);
  }
  return reverse(*head);
}

bool is_list(Obj *obj) { return obj == Nil || obj->type == TCELL; }

static Obj *apply_func(void *root, Obj **env, Obj **fn, Obj **args) {
  DEFINE3(params, newenv, body);
  *params = (*fn)->params;
  *newenv = (*fn)->env;
  *newenv = push_env(root, newenv, params, args);
  *body = (*fn)->body;
  return progn(root, newenv, body);
}

// Apply fn with args.
static Obj *apply(void *root, Obj **env, Obj **fn, Obj **args) {
  if (!is_list(*args)) {
    error("argument must be a list");
    return Nil;
  }
  if ((*fn)->type == TPRIMITIVE)
    return (*fn)->fn(root, env, args);
  if ((*fn)->type == TFUNCTION) {
    DEFINE1(eargs);
    *eargs = eval_list(root, env, args);
    return apply_func(root, env, fn, eargs);
  }
  error("not supported");
  return Nil;
}

Obj *find(Obj **env, Obj *sym) {
  for (Obj *p = *env; p != Nil; p = p->up) {
    for (Obj *cell = p->vars; cell != Nil; cell = cell->cdr) {
      Obj *bind = cell->car;
      if (sym == bind->car)
        return bind;
    }
  }
  return NULL;
}

// Expands the given macro application form.
Obj *macroexpand(void *root, Obj **env, Obj **obj) {
  if ((*obj)->type != TCELL || (*obj)->car->type != TSYMBOL)
    return *obj;
  DEFINE3(bind, macro, args);
  *bind = find(env, (*obj)->car);
  if (!*bind || (*bind)->cdr->type != TMACRO)
    return *obj;
  *macro = (*bind)->cdr;
  *args = (*obj)->cdr;
  return apply_func(root, env, macro, args);
}

Obj *eval(void *root, Obj **env, Obj **obj) {
  switch ((*obj)->type) {
  case TINT:
  case TSTR:
  case TPRIMITIVE:
  case TFUNCTION:
  case TTRUE:
  case TNIL:
    // Self-evaluating objects
    return *obj;
  case TSYMBOL: {
    // Variable
    Obj *bind = find(env, *obj);
    if (!bind) {
      error("Undefined symbol: %s", (*obj)->name);
      return Nil;
    }
    return bind->cdr;
  }
  case TCELL: {
    // Function application form
    DEFINE3(fn, expanded, args);
    *expanded = macroexpand(root, env, obj);
    if (*expanded != *obj)
      return eval(root, env, expanded);
    *fn = (*obj)->car;
    *fn = eval(root, env, fn);
    *args = (*obj)->cdr;
    if ((*fn)->type != TPRIMITIVE && (*fn)->type != TFUNCTION) {
      error("The head of a list must be a function");
      return Nil;
    }
    return apply(root, env, fn, args);
  }
  default:
    error("Bug: eval: Unknown tag type: %d", (*obj)->type);
    return Nil;
  }
}