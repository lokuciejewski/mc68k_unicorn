#include "../inc/memory.h"
#include "../inc/minilisp_int.h"
#include "../minilisp_cfg.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The pointer pointing to the beginning of the old heap
static void *from_space;

// The number of bytes allocated from the heap
static size_t mem_nused = 0;

// The pointer pointing to the beginning of the current heap
static void *memory;

// Flags to debug GC
static bool gc_running = GC_RUNNING;
static bool debug_gc = GC_DEBUG;
static bool always_gc = GC_ALWAYS;

// Round up the given value to a multiple of size. Size must be a power of 2. It
// adds size - 1 first, then zero-ing the least significant bits to make the
// result a multiple of size. I know these bit operations may look a little bit
// tricky, but it's efficient and thus frequently used.
static inline size_t roundup(size_t var, size_t size) {
  return (var + size - 1) & ~(size - 1);
}

// Allocates memory block. This may start GC if we don't have enough memory.
Obj *alloc(void *root, int type, size_t size) {
  // The object must be large enough to contain a pointer for the forwarding
  // pointer. Make it larger if it's smaller than that.
  size = roundup(size, sizeof(void *));

  // Add the size of the type tag and size fields.
  size += offsetof(Obj, value);

  // Round up the object size to the nearest alignment boundary, so that the
  // next object will be allocated at the proper alignment boundary. Currently
  // we align the object at the same boundary as the pointer.
  size = roundup(size, sizeof(void *));

  // If the debug flag is on, allocate a new memory space to force all the
  // existing objects to move to new addresses, to invalidate the old addresses.
  // By doing this the GC behavior becomes more predictable and repeatable. If
  // there's a memory bug that the C variable has a direct reference to a Lisp
  // object, the pointer will become invalid by this GC call. Dereferencing that
  // will immediately cause SEGV.
  if (always_gc && !gc_running)
    gc(root);

  // Otherwise, run GC only when the available memory is not large enough.
  if (!always_gc && MEMORY_SIZE < mem_nused + size)
    gc(root);

  // Terminate the program if we couldn't satisfy the memory request. This can
  // happen if the requested size was too large or the from-space was filled
  // with too many live objects.
  if (MEMORY_SIZE < mem_nused + size) {
    error("Memory exhausted");
    return Nil;
  }

  // Allocate the object.
  Obj *obj = memory + mem_nused;
  obj->type = type;
  obj->size = size;
  mem_nused += size;
  return obj;
}

// Cheney's algorithm uses two pointers to keep track of GC status. At first
// both pointers point to the beginning of the to-space. As GC progresses, they
// are moved towards the end of the to-space. The objects before "scan1" are the
// objects that are fully copied. The objects between "scan1" and "scan2" have
// already been copied, but may contain pointers to the from-space. "scan2"
// points to the beginning of the free space.
static Obj *scan1;
static Obj *scan2;

// Moves one object from the from-space to the to-space. Returns the object's
// new address. If the object has already been moved, does nothing but just
// returns the new address.
static inline Obj *forward(Obj *obj) {
  // If the object's address is not in the from-space, the object is not managed
  // by GC nor it has already been moved to the to-space.
  ptrdiff_t offset = (uint8_t *)obj - (uint8_t *)from_space;
  if (offset < 0 || MEMORY_SIZE <= offset)
    return obj;

  // The pointer is pointing to the from-space, but the object there was a
  // tombstone. Follow the forwarding pointer to find the new location of the
  // object.
  if (obj->type == TMOVED)
    return obj->moved;

  // Otherwise, the object has not been moved yet. Move it.
  Obj *newloc = scan2;
  memcpy(newloc, obj, obj->size);
  scan2 = (Obj *)((uint8_t *)scan2 + obj->size);

  // Put a tombstone at the location where the object used to occupy, so that
  // the following call of forward() can find the object's new location.
  obj->type = TMOVED;
  obj->moved = newloc;
  return newloc;
}

static void *alloc_semispace() { return malloc(MEMORY_SIZE); }

// Copies the root objects.
static void forward_root_objects(void *root) {
  Symbols = forward(Symbols);
  for (void **frame = root; frame; frame = *(void ***)frame)
    for (int i = 1; frame[i] != ROOT_END; i++)
      if (frame[i])
        frame[i] = forward(frame[i]);
}

// Implements Cheney's copying garbage collection algorithm.
// http://en.wikipedia.org/wiki/Cheney%27s_algorithm
void gc(void *root) {
  assert(!gc_running);
  gc_running = true;

  // Allocate a new semi-space.
  from_space = memory;
  memory = alloc_semispace();

  // Initialize the two pointers for GC. Initially they point to the beginning
  // of the to-space.
  scan1 = scan2 = memory;

  // Copy the GC root objects first. This moves the pointer scan2.
  forward_root_objects(root);

  // Copy the objects referenced by the GC root objects located between scan1
  // and scan2. Once it's finished, all live objects (i.e. objects reachable
  // from the root) will have been copied to the to-space.
  while (scan1 < scan2) {
    switch (scan1->type) {
    case TINT:
    case TSTR:
    case TSYMBOL:
    case TPRIMITIVE:
      // Any of the above types does not contain a pointer to a GC-managed
      // object.
      break;
    case TCELL:
      scan1->car = forward(scan1->car);
      scan1->cdr = forward(scan1->cdr);
      break;
    case TFUNCTION:
    case TMACRO:
      scan1->params = forward(scan1->params);
      scan1->body = forward(scan1->body);
      scan1->env = forward(scan1->env);
      break;
    case TENV:
      scan1->vars = forward(scan1->vars);
      scan1->up = forward(scan1->up);
      break;
    default:
      error("Bug: copy: unknown type %d", scan1->type);
    }
    scan1 = (Obj *)((uint8_t *)scan1 + scan1->size);
  }
  // Finish up GC.
  free(from_space);
  size_t old_nused = mem_nused;
  mem_nused = (size_t)((uint8_t *)scan1 - (uint8_t *)memory);
  if (debug_gc)
    fprintf(stderr, "GC: %lu bytes out of %lu bytes copied.\n", mem_nused,
            old_nused);
  gc_running = false;
}

void init_memory(void) {
  // Memory allocation
  memory = alloc_semispace();
}

Obj *make_int(void *root, int value) {
  Obj *r = alloc(root, TINT, sizeof(int));
  r->value = value;
  return r;
}

Obj *cons(void *root, Obj **car, Obj **cdr) {
  Obj *cell = alloc(root, TCELL, sizeof(Obj *) * 2);
  cell->car = *car;
  cell->cdr = *cdr;
  return cell;
}

Obj *make_string(void *root, char *name) {
  Obj *str = alloc(root, TSTR, strlen(name) + 1);
  strcpy(str->name, name);
  return str;
}

Obj *make_symbol(void *root, char *name) {
  Obj *sym = alloc(root, TSYMBOL, strlen(name) + 1);
  strcpy(sym->name, name);
  return sym;
}

Obj *make_primitive(void *root, Primitive *fn) {
  Obj *r = alloc(root, TPRIMITIVE, sizeof(Primitive *));
  r->fn = fn;
  return r;
}

Obj *make_function(void *root, Obj **env, int type, Obj **params,
                          Obj **body) {
  assert(type == TFUNCTION || type == TMACRO);
  Obj *r = alloc(root, type, sizeof(Obj *) * 3);
  r->params = *params;
  r->body = *body;
  r->env = *env;
  return r;
}

struct Obj *make_env(void *root, Obj **vars, Obj **up) {
  Obj *r = alloc(root, TENV, sizeof(Obj *) * 2);
  r->vars = *vars;
  r->up = *up;
  return r;
}

// Returns ((x . y) . a)
Obj *acons(void *root, Obj **x, Obj **y, Obj **a) {
  DEFINE1(cell);
  *cell = cons(root, x, y);
  return cons(root, cell, a);
}
