#include <stdint.h>
#include <stdio.h>

#include "callable.h"
#include "compiler.h"
#include "interpreter.h"
#include "logs.h"
#include "program.h"
#include "vm_def.h"
#include "vm_run.h"

#define error(msg, ...) log_add(NULL, ERROR, program.name, msg, ##__VA_ARGS__)

int interpreter(const char* text_code, uint8_t compile_mode) {
  if (!text_code) {
    error("Codigo invalido");
    return 0;
  }
  CALLABLE_ptr code = compiler(text_code, compile_mode);
  if (!code) {
    return 0;
  }
#ifndef NDEBUG
  printf("== Running ==\n");
#endif
  vm->root_context->ctx_code = code;
  if (vm_run_ctx(vm->root_context) == NULL) {
    callable_free(code);
    return 0;
  }
  callable_free(code);
  log_show_and_clear(NULL);
  return 1;
}
