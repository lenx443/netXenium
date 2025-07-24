#include <stdlib.h>

#include "call_args.h"

CallArgs *call_args_new() {
  CallArgs *new_args = malloc(sizeof(CallArgs));
  if (!new_args) { return NULL; }
  new_args->args = NULL;
  new_args->size = 0;
  new_args->capacity = 0;
  return new_args;
}

int call_args_push(CallArgs *args, struct CArg data) {
  if (args->size >= args->capacity) {
    size_t new_capacity = args->capacity == 0 ? 4 : args->capacity * 2;
    struct CArg **temp_mem = realloc(args->args, sizeof(struct CArg *) * new_capacity);
    if (!temp_mem) { return 1; }
    args->args = temp_mem;
    args->capacity = new_capacity;
  }
  void *data_pointer = malloc(data.size);
  if (!data_pointer) { return 0; }
  memcpy(data_pointer, data.pointer, data.size);
  struct CArg *new_data = malloc(sizeof(struct CArg));
  if (!new_data) {
    free(data_pointer);
    return 0;
  }
  memcpy(new_data,
         &(struct CArg){
             .pointer = data_pointer,
             data.size,
         },
         sizeof(struct CArg));
  args->args[args->size++] = new_data;
  return 1;
}

Unmut_CallArgs *call_args_unmute(CallArgs *args) {}

void call_args_free(CallArgs *args) {}

void call_args_unmute_free(Unmut_CallArgs *args);
