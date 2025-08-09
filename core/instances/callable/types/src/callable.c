#include <stdlib.h>

#include "bytecode.h"
#include "call_args.h"
#include "callable.h"
#include "instance.h"
#include "vm_consts.h"

CALLABLE_ptr callable_new_native(int (*native)(struct __Instance *, CallArgs *)) {
  CALLABLE_ptr new_callable = malloc(sizeof(CALLABLE));
  if (!new_callable) { return NULL; }
  new_callable->callable_type = CALL_NATIVE_FUNCTIIN;
  new_callable->native_callable = native;
  return new_callable;
}

CALLABLE_ptr callable_new_code(ProgramCode_t bpc) {
  CALLABLE_ptr new_callable = malloc(sizeof(CALLABLE));
  if (!new_callable) { return NULL; }
  new_callable->callable_type = CALL_BYTECODE_PROGRAM;
  new_callable->code = bpc;
  return new_callable;
}

void callable_free(CALLABLE_ptr callable) {
  if (!callable) return;
  if (callable->callable_type == CALL_BYTECODE_PROGRAM) {
    bc_free(callable->code.code);
    vm_consts_free(callable->code.consts);
  }
  free(callable);
}
