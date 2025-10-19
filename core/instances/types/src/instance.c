#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "implement.h"
#include "instance.h"
#include "xen_map.h"
#include "xen_nil.h"

struct __Instance *__instance_new(struct __Implement *impl, Xen_INSTANCE *args,
                                  Xen_Instance_Flag flags) {
  if (!impl) { return nil; }
  struct __Instance *inst = malloc(impl->__inst_size);
  if (!inst) { return nil; }
  inst->__refers = 0;
  Xen_ADD_REF(inst);
  inst->__impl = impl;
  inst->__flags = inst->__impl->__inst_default_flags;
  inst->__flags |= flags;
  inst->__size = 0;
  if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    Xen_INSTANCE_MAPPED *mapped = (Xen_INSTANCE_MAPPED *)inst;
    mapped->__map = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
    if (!mapped->__map) {
      free(inst);
      return nil;
    }
  }
  if (impl->__alloc) {
    if (!impl->__alloc(0, inst, args)) {
      if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
        Xen_DEL_REF(((Xen_INSTANCE_MAPPED *)inst)->__map);
      }
      free(inst);
      return nil;
    }
  }
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) { return; }
  if (!XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_STATIC)) {
    if (inst->__impl->__destroy) inst->__impl->__destroy(0, inst, NULL);
    if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
      Xen_DEL_REF(((Xen_INSTANCE_MAPPED *)inst)->__map);
    }
    free(inst);
  }
}
