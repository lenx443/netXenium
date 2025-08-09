#ifndef __CALL_ARGS_H__
#define __CALL_ARGS_H__

#include <stddef.h>

enum CArg_Type { CARG_POINTER, CARG_INSTANCE };

struct CArg {
  enum CArg_Type point_type;
  void *pointer;
  size_t size;
};

struct Callable_Args {
  struct CArg **args;
  size_t size;
  size_t capacity;
};

typedef struct Callable_Args CallArgs;

CallArgs *call_args_new();
int call_args_push(CallArgs *, struct CArg);
void call_args_free(CallArgs *);

#endif
