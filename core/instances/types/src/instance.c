#include "instance.h"
#include "implement.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_map.h"
#include <assert.h>

Xen_Instance* Xen_Instance_Alloc(Xen_Implement* impl) {
  assert(impl != NULL);
  Xen_Instance* inst = (Xen_Instance*)Xen_GC_New(
      impl->__inst_size, impl->__inst_trace, __instance_free);
  inst->__impl = impl;
  inst->__size = 0;
  inst->__flags = 0;
  return inst;
}

struct __Instance* __instance_new(struct __Implement* impl, Xen_INSTANCE* args,
                                  Xen_INSTANCE* kwargs,
                                  Xen_Instance_Flag flags) {
  if (!impl) {
    return NULL;
  }
  struct __Instance* inst;
  if (impl->__alloc) {
    inst = impl->__alloc(0, NULL, args, kwargs);
    if (!inst) {
      return NULL;
    }
  } else {
    inst = Xen_Instance_Alloc(impl);
    if (!inst) {
      return NULL;
    }
    inst->__flags = flags;
    inst->__flags |= impl->__inst_default_flags;
  }
  if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    Xen_INSTANCE_MAPPED* mapped = (Xen_INSTANCE_MAPPED*)inst;
    mapped->__map = Xen_Map_New();
    if (!mapped->__map) {
      Xen_Dealloc(inst);
      return NULL;
    }
  }
  return inst;
}

void __instance_free(Xen_GCHeader** h) {
  Xen_Instance* inst = *(Xen_Instance**)h;
  if (!inst) {
    return;
  }
  if (!XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_STATIC)) {
    if (inst->__impl->__destroy)
      inst->__impl->__destroy(0, inst, NULL, NULL);
    if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    }
    Xen_Dealloc(*h);
  }
}
