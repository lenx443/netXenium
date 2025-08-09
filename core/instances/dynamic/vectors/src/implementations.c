#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "implement.h"
#include "implementations.h"
#include "vm_def.h"

struct __Implementations *__implementations_new() {
  struct __Implementations *impls_new = malloc(sizeof(struct __Implementations));
  if (!impls_new) { return NULL; }
  impls_new->__impls = NULL;
  impls_new->__size = 0;
  impls_new->__capacity = 0;
  return impls_new;
}

bool __implementations_push(struct __Implementations *impls, struct __Implement *impl) {
  if (!impls || !impl) { return false; }
  if (impls->__size >= impls->__capacity) {
    size_t capacity_new = impls->__capacity == 0 ? 4 : impls->__capacity * 2;
    struct __Implement **new_mem =
        realloc(impls->__impls, sizeof(struct __Implement *) * capacity_new);
    if (!new_mem) { return false; }
    impls->__impls = new_mem;
    impls->__capacity = capacity_new;
  }
  impl->__type_index = ++vm->implements_index;
  impls->__impls[impls->__size++] = impl;
  return true;
}

void __implementations_free(struct __Implementations *impls) {
  if (!impls) { return; }
  for (int i = 0; i < impls->__size; i++) {
    __implement_free(impls->__impls[i]);
  }
  free(impls->__impls);
  free(impls);
}
