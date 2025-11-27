#ifndef __CALLABLE_H__
#define __CALLABLE_H__

#include "gc_header.h"
#include "program_code.h"
#include "xen_typedefs.h"

struct __Instance;

#define NATIVE_CLEAR_ARG_NEVER_USE                                             \
  (void)(self);                                                                \
  (void)(args);                                                                \
  (void)(kwargs);
typedef struct __Instance* (*Xen_Native_Func)(struct __Instance*,
                                              struct __Instance*,
                                              struct __Instance*);

struct Callable {
  Xen_GCHeader gc;
  ProgramCode_t code;
};

typedef struct Callable CALLABLE;
typedef CALLABLE* CALLABLE_ptr;

struct Callable_Vector {
  Xen_GCHeader gc;
  struct Callable** callables;
  Xen_size_t count;
  Xen_size_t cap;
};

typedef struct Callable_Vector CALLABLE_Vector;
typedef CALLABLE_Vector* CALLABLE_Vector_ptr;

CALLABLE_ptr callable_new(ProgramCode_t);

CALLABLE_Vector_ptr callable_vector_new(void);
void callable_vector_push(CALLABLE_Vector_ptr, CALLABLE_ptr);
CALLABLE_ptr callable_vector_get(CALLABLE_Vector_ptr, Xen_size_t);

#endif
