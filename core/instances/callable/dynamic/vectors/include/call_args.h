#ifndef __CALL_ARGS_H__
#define __CALL_ARGS_H__

#include <stddef.h>

struct CArg {
  void *pointer;
  size_t size;
};

struct Callable_Args {
  struct CArg **args;
  size_t size;
  size_t capacity;
};

struct Unmut_Callable_Args {
  struct CArg **const args;
  size_t size;
};

typedef struct Callable_Args CallArgs;
typedef struct Unmut_Callable_Args Unmut_CallArgs;

CallArgs *call_args_new();
int call_args_push(CallArgs *, struct CArg);
Unmut_CallArgs *const call_args_unmute(CallArgs *);
void call_args_free(CallArgs *);
void call_args_unmute_free(Unmut_CallArgs *);

#endif
