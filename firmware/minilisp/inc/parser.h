/// Parser API
#pragma once
#include "minilisp_int.h"
#include <stdbool.h>

Obj *macroexpand(void *root, Obj **env, Obj **obj);

// Evaluates the S expression.
Obj *eval(void *root, Obj **env, Obj **obj);

// Evaluates the list elements from head and returns the last return value.
Obj *progn(void *root, Obj **env, Obj **list);

// May create a new symbol. If there's a symbol with the same name, it will not
// create a new symbol but return the existing one.
Obj *intern(void *root, char *name);

Obj *eval_list(void *root, Obj **env, Obj **list);

void add_variable(void *root, Obj **env, Obj **sym, Obj **val);

// Searches for a variable by symbol. Returns null if not found.
Obj *find(Obj **env, Obj *sym);

Obj *read_expr(void *root);

// Returns the length of the given list. -1 if it's not a proper list.
int length(Obj *list);

bool is_list(Obj *obj);

// Prints the given object.
void print(Obj *obj);