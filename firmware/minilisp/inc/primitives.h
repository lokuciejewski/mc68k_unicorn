/// All primitives declarations
#pragma once

#include "minilisp_int.h"

void define_primitives(void *root, Obj **env);

void define_constants(void *root, Obj **env);

Obj *prim_quote(void *root, Obj **env, Obj **list);

Obj *prim_cons(void *root, Obj **env, Obj **list);

Obj *prim_car(void *root, Obj **env, Obj **list);

Obj *prim_cdr(void *root, Obj **env, Obj **list);

Obj *prim_setq(void *root, Obj **env, Obj **list);

Obj *prim_setcar(void *root, Obj **env, Obj **list);

Obj *prim_while(void *root, Obj **env, Obj **list);

Obj *prim_gensym(void *root, Obj **env, Obj **list);

Obj *prim_plus(void *root, Obj **env, Obj **list);

Obj *prim_minus(void *root, Obj **env, Obj **list);

Obj *prim_mult(void *root, Obj **env, Obj **list);

Obj *prim_shl(void *root, Obj **env, Obj **list);

Obj *prim_shr(void *root, Obj **env, Obj **list);

Obj *prim_lt(void *root, Obj **env, Obj **list);

Obj *handle_function(void *root, Obj **env, Obj **list, int type);

Obj *prim_lambda(void *root, Obj **env, Obj **list);

Obj *prim_defun(void *root, Obj **env, Obj **list);

Obj *prim_define(void *root, Obj **env, Obj **list);

Obj *prim_defmacro(void *root, Obj **env, Obj **list);

Obj *prim_macroexpand(void *root, Obj **env, Obj **list);

Obj *prim_println(void *root, Obj **env, Obj **list);

Obj *prim_if(void *root, Obj **env, Obj **list);

Obj *prim_num_eq(void *root, Obj **env, Obj **list);

Obj *prim_eq(void *root, Obj **env, Obj **list);

Obj *prim_rmem(void *root, Obj **env, Obj **list);

Obj *prim_wmem(void *root, Obj **env, Obj **list);

Obj *prim_hex(void *root, Obj **env, Obj **list);