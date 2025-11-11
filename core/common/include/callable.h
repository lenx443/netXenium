#ifndef __CALLABLE_H__
#define __CALLABLE_H__

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

enum Callable_Type {
  CALL_NATIVE_FUNCTIIN,
  CALL_BYTECODE_PROGRAM,
};

struct Callable {
  enum Callable_Type callable_type;
  union {
    Xen_Native_Func native_callable;
    ProgramCode_t code;
  };
};

typedef enum Callable_Type CALLABLE_TYPE;
typedef struct Callable CALLABLE;
typedef CALLABLE* CALLABLE_ptr;

CALLABLE_ptr callable_new_native(Xen_Native_Func);
CALLABLE_ptr callable_new_code(ProgramCode_t);
void callable_free(CALLABLE_ptr);

#endif
