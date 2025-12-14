#include "instance.h"
#include "implement.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"

Xen_Instance* Xen_Instance_Alloc(Xen_Implement* impl) {
  assert(impl != NULL);
  Xen_Instance* inst = (Xen_Instance*)Xen_GC_New(
      impl->__inst_size, impl->__inst_trace, __instance_free);
  inst->__impl = impl;
  inst->__size = 0;
  inst->__flags = impl->__inst_default_flags;
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
    inst = impl->__alloc(NULL, args, kwargs);
    if (!inst) {
      return NULL;
    }
    Xen_IGC_Push(inst);
  } else {
    inst = Xen_Instance_Alloc(impl);
    Xen_IGC_Push(inst);
    inst->__flags = flags;
    inst->__flags |= impl->__inst_default_flags;
  }
  if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    Xen_INSTANCE_MAPPED* mapped = (Xen_INSTANCE_MAPPED*)inst;
    mapped->__map = Xen_Map_New();
    if (!mapped->__map) {
      Xen_IGC_Pop();
      return NULL;
    }
    if (impl->__base) {
      Xen_Instance* base = __instance_new(impl->__base, nil, nil, 0);
      if (!base) {
        Xen_IGC_Pop();
        return NULL;
      }
      if (!Xen_Map_Push_Pair_Str(mapped->__map,
                                 (Xen_Map_Pair_Str){"0base", base})) {
        Xen_IGC_Pop();
        return NULL;
      }
    }
  }
  Xen_IGC_Pop();
  return inst;
}

void __instance_free(Xen_GCHeader** h) {
  Xen_Instance* inst = *(Xen_Instance**)h;
  if (!inst) {
    return;
  }
  if (!XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_STATIC)) {
    if (inst->__impl->__destroy)
      inst->__impl->__destroy(inst, NULL, NULL);
    Xen_Dealloc(*h);
  }
}
