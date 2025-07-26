#ifndef __CALLABLE_H__
#define __CALLABLE_H__

#include "call_args.h"
#include "list.h"
#include "program_code.h"

struct __Instance;

enum Callable_Type {
  CALL_NATIVE_FUNCTIIN,
  CALL_BYTECODE_PROGRAM,
};

struct Callable {
  enum Callable_Type callable_type;
  union {
    int (*native_callable)(struct __Instance *, Unmut_CallArgs *const);
    ProgramCode_t code;
  };
};

typedef enum Callable_Type CALLABLE_TYPE;
typedef struct Callable CALLABLE;
typedef CALLABLE *CALLABLE_ptr;

CALLABLE_ptr callable_new_native(int (*)(LIST_ptr));
CALLABLE_ptr callable_new_code(ProgramCode_t);

#endif
