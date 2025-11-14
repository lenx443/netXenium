#include <stdint.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_alloc.h"
#include "xen_typedefs.h"

void* Xen_Alloc(Xen_size_t size) {
  return malloc(size);
}

void* Xen_Realloc(void* mem, Xen_size_t size) {
  return realloc(mem, size);
}

void Xen_Dealloc(void* mem) {
  free(mem);
}

Xen_Instance* Xen_Instance_Alloc(Xen_Implement* impl) {
  if (!impl) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Alloc(impl->__inst_size);
  if (!inst) {
    return NULL;
  }
  inst->__refers = 1;
  inst->__impl = impl;
  inst->__size = 0;
  inst->__flags = 0;
  return inst;
}
