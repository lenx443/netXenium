#ifndef __CALLABLE_H__
#define __CALLABLE_H__

#include "gc_header.h"
#include "program_code.h"

struct __Instance;

#define NATIVE_CLEAR_ARG_NEVER_USE                                             \
  (void)(id);                                                                  \
  (void)(self);                                                                \
  (void)(args);                                                                \
  (void)(kwargs);
typedef struct __Instance* (*Xen_Native_Func)(unsigned long, struct __Instance*,
                                              struct __Instance*,
                                              struct __Instance*);

struct Callable {
  Xen_GCHeader gc;
  ProgramCode_t code;
};

typedef struct Callable CALLABLE;
typedef CALLABLE* CALLABLE_ptr;

CALLABLE_ptr callable_new(ProgramCode_t);

#endif
