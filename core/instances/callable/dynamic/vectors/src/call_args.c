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
  new_data->pointer = data_pointer;
  new_data->size = data.size;
  args->args[args->size++] = new_data;
  return 1;
}

Unmut_CallArgs *call_args_unmute(CallArgs *args) {
  struct CArg **const new_args = malloc(sizeof(struct CArg *) * args->size);
  if (!new_args) { return NULL; }
  for (size_t i = 0; i < args->size; i++) {
    struct CArg *src = args->args[i];
    struct CArg *copy = malloc(sizeof(struct CArg));
    if (!copy) {
      for (size_t j = 0; j < i; j++) {
        free(new_args[j]->pointer);
        free(new_args[j]);
      }
      free(new_args);
      return NULL;
    }

    copy->size = src->size;

    if (src->pointer && src->size > 0) {
      copy->pointer = malloc(src->size);
      if (!copy->pointer) {
        free(copy);
        for (size_t j = 0; j < i; j++) {
          free(new_args[j]->pointer);
          free(new_args[j]);
        }
        free(new_args);
        return NULL;
      }
      memcpy(copy->pointer, src->pointer, src->size);
    } else {
      copy->pointer = NULL;
    }

    new_args[i] = copy;
  }
  Unmut_CallArgs *new_unmutable = malloc(sizeof(Unmut_CallArgs));
  if (!new_unmutable) {
    free(new_args);
    return NULL;
  }
  *(struct CArg ***)&new_unmutable->args = new_args;
  new_unmutable->size = args->size;
  call_args_free(args);
  return new_unmutable;
}

#define FREE_ARGS(args)                                                                  \
  if (!args) return;                                                                     \
  for (int i = 0; i < args->size; i++) {                                                 \
    free(args->args[i]->pointer);                                                        \
    free(args->args[i]);                                                                 \
  }                                                                                      \
  free(args->args);                                                                      \
  free(args);

void call_args_free(CallArgs *args) { FREE_ARGS(args) }

void call_args_unmute_free(Unmut_CallArgs *args) { FREE_ARGS(args) }
