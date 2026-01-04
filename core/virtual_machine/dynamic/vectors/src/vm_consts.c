#include <stdbool.h>
#include <stddef.h>

#include "callable.h"
#include "gc_header.h"
#include "instance.h"
#include "vm_consts.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static void vm_consts_trace(Xen_GCHeader* h) {
  vm_Consts_ptr consts = (vm_Consts_ptr)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)consts->c_names->ptr);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)consts->c_instances->ptr);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)consts->c_callables->ptr);
}

static void vm_consts_destroy(Xen_GCHeader* h) {
  vm_Consts_ptr consts = (vm_Consts_ptr)h;
  Xen_GCHandle_Free(consts->c_names);
  Xen_GCHandle_Free(consts->c_instances);
  Xen_GCHandle_Free(consts->c_callables);
  Xen_Dealloc(h);
}

vm_Consts_ptr vm_consts_new(void) {
  vm_Consts_ptr consts = (vm_Consts_ptr)Xen_GC_New(
      sizeof(vm_Consts), vm_consts_trace, vm_consts_destroy);
  if (!consts) {
    return NULL;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)consts);
  Xen_Instance* c_names = Xen_Vector_New();
  if (!c_names) {
    Xen_GC_Pop_Root();
    return NULL;
  }
  Xen_Instance* c_instances = Xen_Vector_New();
  if (!c_instances) {
    Xen_GC_Pop_Root();
    return NULL;
  }
  if (!Xen_Vector_Push(c_instances, nil)) {
    Xen_GC_Pop_Root();
    return NULL;
  }
  if (!Xen_Vector_Push(c_instances, Xen_True)) {
    Xen_GC_Pop_Root();
    return NULL;
  }
  if (!Xen_Vector_Push(c_instances, Xen_False)) {
    Xen_GC_Pop_Root();
    return NULL;
  }
  CALLABLE_Vector_ptr c_callables = callable_vector_new();
  consts->c_names = Xen_GCHandle_New();
  consts->c_instances = Xen_GCHandle_New();
  consts->c_callables = Xen_GCHandle_New();
  Xen_GC_Write_Field((Xen_GCHeader*)consts, (Xen_GCHandle**)&consts->c_names,
                     (Xen_GCHeader*)c_names);
  Xen_GC_Write_Field((Xen_GCHeader*)consts,
                     (Xen_GCHandle**)&consts->c_instances,
                     (Xen_GCHeader*)c_instances);
  Xen_GC_Write_Field((Xen_GCHeader*)consts,
                     (Xen_GCHandle**)&consts->c_callables,
                     (Xen_GCHeader*)c_callables);
  Xen_GC_Pop_Root();
  return consts;
}

vm_Consts_ptr vm_consts_from_values(struct __Instance* c_names,
                                    struct __Instance* c_instances,
                                    CALLABLE_Vector_ptr c_callables) {
  vm_Consts_ptr consts = (vm_Consts_ptr)Xen_GC_New(
      sizeof(vm_Consts), vm_consts_trace, vm_consts_destroy);
  if (!consts) {
    return NULL;
  }
  consts->c_names = Xen_GCHandle_New();
  consts->c_instances = Xen_GCHandle_New();
  consts->c_callables = Xen_GCHandle_New();
  Xen_GC_Write_Field((Xen_GCHeader*)consts, (Xen_GCHandle**)&consts->c_names,
                     (Xen_GCHeader*)c_names);
  Xen_GC_Write_Field((Xen_GCHeader*)consts,
                     (Xen_GCHandle**)&consts->c_instances,
                     (Xen_GCHeader*)c_instances);
  Xen_GC_Write_Field((Xen_GCHeader*)consts,
                     (Xen_GCHandle**)&consts->c_callables,
                     (Xen_GCHeader*)c_callables);
  return consts;
}

Xen_ssize_t vm_consts_push_name(vm_Consts_ptr consts, const char* c_name) {
  if (!consts || !c_name) {
    return false;
  }
  Xen_Instance* c_name_inst = Xen_String_From_CString(c_name);
  if (!c_name_inst) {
    return false;
  }
  Xen_size_t index = Xen_SIZE(consts->c_names);
  if (!Xen_Vector_Push((Xen_Instance*)consts->c_names->ptr, c_name_inst)) {
    return -1;
  }
  return index;
}

Xen_ssize_t vm_consts_push_instance(vm_Consts_ptr consts,
                                    struct __Instance* c_instance) {
  if (!consts || !c_instance) {
    return false;
  };
  if (c_instance == nil) {
    return 0;
  }
  if (c_instance == Xen_True) {
    return 1;
  }
  if (c_instance == Xen_False) {
    return 2;
  }
  Xen_size_t index = Xen_SIZE(consts->c_instances);
  if (!Xen_Vector_Push((Xen_Instance*)consts->c_instances->ptr, c_instance)) {
    return -1;
  }
  return index;
}

Xen_ssize_t vm_consts_push_callable(vm_Consts_ptr consts,
                                    CALLABLE_ptr callable) {
  if (!consts || !callable) {
    return false;
  }
  Xen_size_t index = ((CALLABLE_Vector*)consts->c_callables->ptr)->count;
  callable_vector_push((CALLABLE_Vector*)consts->c_callables->ptr, callable);
  return index;
}
