/// Memory API
#pragma once

#include "minilisp_int.h"
#include <stddef.h>

void init_memory(void);

void gc(void *root);

Obj *alloc(void *root, int type, size_t size);

// Currently we are using Cheney's copying GC algorithm, with which the
// available memory is split into two halves and all objects are moved from one
// half to another every time GC is invoked. That means the address of the
// object keeps changing. If you take the address of an object and keep it in a
// C variable, dereferencing it could cause SEGV because the address becomes
// invalid after GC runs.
//
// In order to deal with that, all access from C to Lisp objects will go through
// two levels of pointer dereferences. The C local variable is pointing to a
// pointer on the C stack, and the pointer is pointing to the Lisp object. GC is
// aware of the pointers in the stack and updates their contents with the
// objects' new addresses when GC happens.
//
// The following is a macro to reserve the area in the C stack for the pointers.
// The contents of this area are considered to be GC root.
//
// Be careful not to bypass the two levels of pointer indirections. If you
// create a direct pointer to an object, it'll cause a subtle bug. Such code
// would work in most cases but fails with SEGV if GC happens during the
// execution of the code. Any code that allocates memory may invoke GC.

#define ROOT_END ((void *)-1)

#define ADD_ROOT(size)                                                         \
  void *root_ADD_ROOT_[size + 2];                                              \
  root_ADD_ROOT_[0] = root;                                                    \
  for (int i = 1; i <= size; i++)                                              \
    root_ADD_ROOT_[i] = NULL;                                                  \
  root_ADD_ROOT_[size + 1] = ROOT_END;                                         \
  root = root_ADD_ROOT_

#define DEFINE1(var1)                                                          \
  ADD_ROOT(1);                                                                 \
  Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1)

#define DEFINE2(var1, var2)                                                    \
  ADD_ROOT(2);                                                                 \
  Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);                                   \
  Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2)

#define DEFINE3(var1, var2, var3)                                              \
  ADD_ROOT(3);                                                                 \
  Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);                                   \
  Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2);                                   \
  Obj **var3 = (Obj **)(root_ADD_ROOT_ + 3)

#define DEFINE4(var1, var2, var3, var4)                                        \
  ADD_ROOT(4);                                                                 \
  Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);                                   \
  Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2);                                   \
  Obj **var3 = (Obj **)(root_ADD_ROOT_ + 3);                                   \
  Obj **var4 = (Obj **)(root_ADD_ROOT_ + 4)

//======================================================================
// Constructors
//======================================================================

Obj *make_int(void *root, int value);

Obj *cons(void *root, Obj **car, Obj **cdr);

Obj *make_string(void *root, char *name);

Obj *make_symbol(void *root, char *name);

Obj *make_primitive(void *root, Primitive *fn);

Obj *make_function(void *root, Obj **env, int type, Obj **params, Obj **body);

struct Obj *make_env(void *root, Obj **vars, Obj **up);

Obj *acons(void *root, Obj **x, Obj **y, Obj **a);
