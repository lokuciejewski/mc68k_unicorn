/**
BSD 3-Clause License

Copyright (c) 2021, Robert van Engelen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* tinylisp-opt-gc.c optimized and ref count garbage collection and error
 * handling by Robert A. van Engelen 2025 */
#include "hal/inc/memory.h"
#include "hal/inc/spi.h"
#include "hal/inc/time.h"
#include "middleware/api/sd_card.h"
#include <math.h> /* to return NAN from num() */
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I unsigned
#define L double
#define A (char *)cell
#define N 8192

I ref[N / 2], hp, fp, lp, fn, ATOM = 0x7ff8, PRIM = 0x7ff9, CONS = 0x7ffa,
                              CLOS = 0x7ffb, NIL = 0x7ffc;
L cell[N], nil, tru, env;

jmp_buf jb; /* we longjmp(jb,1) to REPL on memory overflow then mark-sweep
               garbage collect, instead of abort() */
L err(I i) { longjmp(jb, i); }

I T(L x) {
  union {
    L x;
    unsigned long long i;
  } u = {x};
  return u.i >> 48;
}

L box(I t, I i) {
  union {
    unsigned long long i;
    L x;
  } u = {(unsigned long long)t << 48 | i};
  return u.x;
}

I ord(L x) {
  union {
    L x;
    unsigned long long i;
  } u = {x};
  return u.i;
}

L num(L n) { return n == n ? n : NAN; }

I equ(L x, L y) {
  union {
    L x;
    unsigned long long i;
  } u = {x}, v = {y};
  return u.i == v.i;
}

L atom(const char *s) {
  I i = 0;
  while (i < hp && strcmp(A + i, s))
    i += strlen(A + i) + 1;
  return i == hp && (hp += strlen(strcpy(A + i, s)) + 1) > lp << 3
             ? err(4)
             : box(ATOM, i);
}

I lomem(I i) { return lp = i < lp ? i : lp; }

L alloc() {
  I i = fp;
  fp = ref[i / 2];
  ref[i / 2] = 1;
  --fn;
  return hp > lomem(i) << 3 ? err(4) : box(CONS, i);
}

L cons(L x, L y) {
  L p = alloc();
  cell[ord(p) + 1] = x;
  cell[ord(p)] = y;
  return p;
}

L car(L p) {
  return (T(p) & ~(CONS ^ CLOS)) == CONS ? cell[ord(p) + 1] : err(1);
}

L cdr(L p) { return (T(p) & ~(CONS ^ CLOS)) == CONS ? cell[ord(p)] : err(1); }

L pair(L v, L x, L e) { return cons(cons(v, x), e); }

L closure(L v, L x, L e) { return box(CLOS, ord(pair(v, x, e))); }

L assoc(L v, L e) {
  while (T(e) == CONS && !equ(v, car(car(e))))
    e = cdr(e);
  return T(e) == CONS ? cdr(car(e)) : err(2);
}

I not(L x) { return T(x) == NIL; }

I let(L x) { return !not(x) && !not(cdr(x)); }

L dup(L x) {
  if ((T(x) & ~(CONS ^ CLOS)) == CONS)
    ++ref[ord(x) / 2];
  return x;
}

void del(I i) {
  ref[i / 2] = fp;
  fp = i;
  ++fn;
}

void gc(L x) {
  I i;
  if ((T(x) & ~(CONS ^ CLOS)) == CONS && !--ref[(i = ord(x)) / 2]) {
    del(i);
    gc(cell[i + 1]);
    gc(cell[i]);
  }
}

L eval(L, L), parse();

L evlis(L t, L e) {
  L s, *p;
  for (s = nil, p = &s; T(t) == CONS; p = &cell[ord(*p)], t = cdr(t))
    *p = cons(eval(car(t), e), nil);
  if (T(t) == ATOM)
    *p = dup(assoc(t, e));
  return s;
}

L evarg(L *t, L *e, I *a) {
  L x;
  if (T(*t) == ATOM && !*a)
    *t = assoc(*t, *e), *a = 1;
  x = car(*t);
  *t = cdr(*t);
  return *a ? dup(x) : eval(x, *e);
}

I isarg(L *t, L *e, I *a, L *x) {
  if (T(*t) == ATOM && !*a)
    *t = assoc(*t, *e), *a = 1;
  if (not(*t))
    return 0;
  *x = car(*t);
  *t = cdr(*t);
  *x = *a ? dup(*x) : eval(*x, *e);
  return 1;
}

L f_eval(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = eval(x, *e);
  gc(x);
  return y;
}

L f_quote(L t, L *_) { return dup(car(t)); }

L f_cons(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  return cons(x, evarg(&t, e, &a));
}

L f_car(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = dup(car(x));
  gc(x);
  return y;
}

L f_cdr(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = dup(cdr(x));
  gc(x);
  return y;
}

L f_add(L t, L *e) {
  I a = 0;
  L x, n = evarg(&t, e, &a);
  for (gc(n); isarg(&t, e, &a, &x); gc(x))
    n += x;
  return num(n);
}

L f_sub(L t, L *e) {
  I a = 0;
  L x, n = evarg(&t, e, &a);
  for (gc(n); isarg(&t, e, &a, &x); gc(x))
    n -= x;
  return num(n);
}

L f_mul(L t, L *e) {
  I a = 0;
  L x, n = evarg(&t, e, &a);
  for (gc(n); isarg(&t, e, &a, &x); gc(x))
    n *= x;
  return num(n);
}

L f_div(L t, L *e) {
  I a = 0;
  L x, n = evarg(&t, e, &a);
  for (gc(n); isarg(&t, e, &a, &x); gc(x))
    n /= x;
  return num(n);
}

L f_int(L t, L *e) {
  I a = 0;
  L n = evarg(&t, e, &a);
  gc(n);
  return n < 1e16 && n > -1e16 ? (long long)n : n;
}

L f_lt(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = evarg(&t, e, &a);
  gc(x);
  gc(y);
  return x - y < 0 ? tru : nil;
}

L f_eq(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a), y = evarg(&t, e, &a);
  gc(x);
  gc(y);
  return equ(x, y) ? tru : nil;
}

L f_pair(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  gc(x);
  return T(x) == CONS ? tru : nil;
}

L f_not(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  gc(x);
  return not(x) ? tru : nil;
}

L f_or(L t, L *e) {
  I a = 0;
  L x = nil;
  while (isarg(&t, e, &a, &x) && not(x))
    continue;
  return x;
}

L f_and(L t, L *e) {
  I a = 0;
  L x, y = tru;
  for (; isarg(&t, e, &a, &x) && !not(x); y = x)
    gc(y);
  return y;
}

L f_cond(L t, L *e) {
  L x;
  while (x = eval(car(car(t)), *e), gc(x), not(x))
    t = cdr(t);
  return car(cdr(car(t)));
}

L f_if(L t, L *e) {
  L x, y = car(cdr(not(x = eval(car(t), *e)) ? cdr(t) : t));
  gc(x);
  return y;
}

L f_leta(L t, L *e) {
  for (; let(t); t = cdr(t))
    *e = pair(car(car(t)), eval(car(cdr(car(t))), *e), *e);
  return car(t);
}

L f_lambda(L t, L *e) {
  return closure(dup(car(t)), dup(car(cdr(t))), equ(*e, env) ? nil : dup(*e));
}

L f_define(L t, L *e) {
  env = pair(car(t), eval(car(cdr(t)), *e), env);
  return car(t);
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

L f_spi_w(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  if (x > UINT8_MAX) {
    return nil;
  }
  gc(x);
  return num(SPI_WriteByte(x));
}

L f_spi_r(L t, L *e) { return num(SPI_ReadByte()); }

L f_delay_ms(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  gc(x);
  delay_ms(x);
  return nil;
}

L f_sd_init(L t, L *e) { return num(SD_Init()); }

L f_sd_type(L t, L *e) { return num(SD_Type()); }

static void blockdump(uint8_t *buffer, uint32_t len) {
  printf("\r\n");
  char ascii[17];
  ascii[16] = '\0';

  for (uint16_t i = 0; i < len; i++) {
    if (i % 16 == 0) {
      if (i != 0)
        printf(" |%s|\n", ascii);
      printf("0x%04x: ", i);
    }

    uint8_t b = buffer[i];
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

L f_sd_read(L t, L *e) {
  I a = 0;
  L x = evarg(&t, e, &a);
  gc(x);
  uint8_t buffer[513] = {0};
  SD_RetCode_e ret = SD_ReadBlock(x, buffer);
  blockdump(buffer, 512);
  return num(ret);
}

struct {
  const char *s;
  L (*f)(L, L *);
  short t;
} prim[] = {{"eval", f_eval, 0},
            {"quote", f_quote, 0},
            {"cons", f_cons, 0},
            {"car", f_car, 0},
            {"cdr", f_cdr, 0},
            {"+", f_add, 0},
            {"-", f_sub, 0},
            {"*", f_mul, 0},
            {"/", f_div, 0},
            {"<<", f_lsh, 0},
            {"int", f_int, 0},
            {"<", f_lt, 0},
            {"eq?", f_eq, 0},
            {"or", f_or, 0},
            {"and", f_and, 0},
            {"not", f_not, 0},
            {"cond", f_cond, 1},
            {"if", f_if, 1},
            {"let*", f_leta, 1},
            {"lambda", f_lambda, 0},
            {"define", f_define, 0},
            {"pair?", f_pair, 0},
            {"rmem", f_rmem, 0},
            {"wmem", f_wmem, 0},
            {"rspi", f_spi_r, 0},
            {"wspi", f_spi_w, 0},
            {"sd_init", f_sd_init, 0},
            {"sd_type", f_sd_type, 0},
            {"sd_read", f_sd_read, 0},
            {"delay", f_delay_ms, 0},
            {0}};

L eval(L x, L e) {
  I a;
  L f = nil, g, v, d;
  dup(e);
  while (1) {
    if (T(x) == ATOM) {
      x = dup(assoc(x, e));
      break;
    }
    if (T(x) != CONS) {
      x = dup(x);
      break;
    }
    g = f;
    f = eval(car(x), e);
    x = cdr(x);
    if (T(f) == PRIM) {
      x = prim[ord(f)].f(x, &e);
      gc(g);
      if (prim[ord(f)].t)
        continue;
      break;
    }
    if (T(f) != CLOS)
      err(3);
    v = car(car(f));
    d = dup(not(cdr(f)) ? env : cdr(f));
    for (a = 0; T(v) == CONS; v = cdr(v))
      d = pair(car(v), evarg(&x, &e, &a), d);
    if (T(v) == ATOM)
      d = pair(v, a ? dup(x) : evlis(x, e), d);
    x = cdr(car(f));
    gc(e);
    e = d;
    gc(g);
  }
  gc(e);
  gc(f);
  return x;
}

char buf[40], see = 0;
void look() {
  int c = getchar();
  see = c;
  if (c == EOF)
    exit(0);
}

I seeing(char c) { return c == ' ' ? see >= 0 && see <= c : see == c; }
char get() {
  char c = see;
  look();
  return c;
}

char scan() {
  int i = 0;
  while (seeing(' '))
    look();
  if (seeing('(') || seeing(')') || seeing('\''))
    buf[i++] = get();
  else
    do
      buf[i++] = get();
    while (i < 39 && !seeing('(') && !seeing(')') && !seeing(' '));
  return buf[i] = 0, *buf;
}

L Read() { return scan(), parse(); }

L list() {
  L t, *p;
  for (t = nil, p = &t;; *p = cons(parse(), nil), p = &cell[ord(*p)]) {
    if (scan() == ')')
      return t;
    if (*buf == '.' && !buf[1])
      return *p = Read(), scan(), t;
  }
}

L parse() {
  L n;
  int i;
  if (*buf == '(')
    return list();
  if (*buf == '\'')
    return cons(atom("quote"), cons(Read(), nil));
  return sscanf(buf, "%lg%n", &n, &i) > 0 && !buf[i] ? n : atom(buf);
}

void print(L);

void printlist(L t) {
  for (putchar('(');; putchar(' ')) {
    print(car(t));
    if (not(t = cdr(t)))
      break;
    if (T(t) != CONS) {
      printf(" . ");
      print(t);
      break;
    }
  }
  putchar(')');
}

void print(L x) {
  if (T(x) == NIL)
    printf("()");
  else if (T(x) == ATOM)
    printf("%s", A + ord(x));
  else if (T(x) == PRIM)
    printf("<%s>", prim[ord(x)].s);
  else if (T(x) == CONS)
    printlist(x);
  else if (T(x) == CLOS)
    printf("{%u}", ord(x));
  else
    printf("%.10lg", x);
}

void mark(L x) {
  if ((T(x) & ~(CONS ^ CLOS)) == CONS && !ref[ord(x) / 2]++) {
    mark(cell[ord(x) + 1]);
    mark(cell[ord(x)]);
  }
}

void sweep() {
  I i;
  for (hp = 0, i = 0; i < N; ++i)
    if (ref[i / 2] && T(cell[i]) == ATOM && ord(cell[i]) > hp)
      hp = ord(cell[i]);
  if (hp)
    hp += strlen(A + hp) + 1;
  for (fp = 0, lp = N - 2, fn = 1, i = 2; i < N; i += 2)
    if (ref[i / 2])
      lomem(i);
    else
      del(i);
}

void rebuild() {
  memset(ref, 0, sizeof(ref));
  mark(env);
  sweep();
}

int tinylisp() {
  I i;
  printf("tinylisp-opt-gc");
  env = 0;
  rebuild();
  nil = box(NIL, 0);
  atom("ERR");
  tru = atom("#t");
  env = pair(tru, tru, nil);
  for (i = 0; prim[i].s; ++i)
    env = pair(atom(prim[i].s), box(PRIM, i), env);
  if ((i = setjmp(jb)) > 0)
    printf("\e[31;1mERR %u%s\e[m", i, i == 4 ? " out of memory" : "");
  while (1) {
    L x, y;
    rebuild();
    printf("\n%u>", 2 * fn - hp / 8);
    print(y = eval(x = Read(), env));
    gc(y);
    gc(x);
  }
}