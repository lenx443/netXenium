#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xen_alloc.h"
#include "xen_typedefs.h"

void* Xen_Alloc(Xen_size_t size) {
  void* mem = malloc(size);
  if (!mem) {
    puts("Fatal: No memory");
    abort();
  }
  return mem;
}

void* Xen_ZAlloc(Xen_size_t count, Xen_size_t item_size) {
  void* mem = malloc(count * item_size);
  if (!mem) {
    puts("Fatal: No memory");
    abort();
  }
  memset(mem, 0, count * item_size);
  return mem;
}

void* Xen_Realloc(void* mem, Xen_size_t size) {
  if (size == 0) {
    free(mem);
    return NULL;
  }
  void* new_mem = realloc(mem, size);
  if (!new_mem) {
    puts("Fatal: No memory");
    abort();
  }
  return new_mem;
}

void Xen_Dealloc(void* mem) {
  free(mem);
}
