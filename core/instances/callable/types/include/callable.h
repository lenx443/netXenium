#ifndef __CALLABLE_H__
#define __CALLABLE_H__

#include "program_code.h"

struct __Instance;

enum Callable_Type {
  CALL_NATIVE_FUNCTIIN,
  CALL_BYTECODE_PROGRAM,
};

struct Callable {
  enum Callable_Type callable_type;
  union {
    void (*native_callable)();
    ProgramCode_t code;
  };
};

typedef enum Callable_Type CALLABLE_TYPE;
typedef struct Callable CALLABLE;
typedef CALLABLE *CALLABLE_ptr;

CALLABLE_ptr callable_new_native(void (*)());
CALLABLE_ptr callable_new_code(ProgramCode_t);
void callable_free(CALLABLE_ptr);

#endif
