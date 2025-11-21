#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xen_alloc.h"
#include "xen_typedefs.h"

void* Xen_Alloc(Xen_size_t size) {
  return malloc(size);
}

void* Xen_ZAlloc(Xen_size_t count, Xen_size_t item_size) {
  void* mem = malloc(count * item_size);
  if (!mem) {
    return NULL;
  }
  memset(mem, 0, count * item_size);
  return mem;
}

void* Xen_Realloc(void* mem, Xen_size_t size) {
  return realloc(mem, size);
}

void Xen_Dealloc(void* mem) {
  free(mem);
}
