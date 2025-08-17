#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "call_args.h"
#include "instance.h"

CallArgs *call_args_new() {
  CallArgs *new_args = malloc(sizeof(CallArgs));
  if (!new_args) { return NULL; }
  new_args->args = NULL;
  new_args->size = 0;
  new_args->capacity = 0;
  return new_args;
}

CallArgs *call_args_create(size_t count, struct CArg *args) {
  CallArgs *cargs = call_args_new();
  if (!cargs) { return NULL; }
  for (int i = 0; i < count; i++) {
    struct CArg carg = args[i];
    if (!call_args_push(cargs, carg)) {
      call_args_free(cargs);
      return NULL;
    }
  }
  return cargs;
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
  if (data.pointer == NULL)
    data_pointer = NULL;
  else if (data.point_type == CARG_INSTANCE) {
    data_pointer = data.pointer;
    Xen_ADD_REF(data.pointer);
  } else
    memcpy(data_pointer, data.pointer, data.size);
  struct CArg *new_data = malloc(sizeof(struct CArg));
  if (!new_data) {
    free(data_pointer);
    return 0;
  }
  new_data->point_type = data.point_type;
  new_data->pointer = data_pointer;
  new_data->size = data.size;
  args->args[args->size++] = new_data;
  return 1;
}

void call_args_free(CallArgs *args) {
  if (!args) return;
  for (int i = 0; i < args->size; i++) {
    if (args->args[i]->pointer) {
      if (args->args[i]->point_type == CARG_INSTANCE)
        Xen_DEL_REF(args->args[i]->pointer);
      else
        free(args->args[i]->pointer);
    }
    free(args->args[i]);
  }
  free(args->args);
  free(args);
}
