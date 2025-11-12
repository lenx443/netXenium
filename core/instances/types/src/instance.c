#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "implement.h"
#include "instance.h"
#include "xen_map.h"

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
    inst = malloc(impl->__inst_size);
    if (!inst) {
      return NULL;
    }
    inst->__refers = 0;
    Xen_ADD_REF(inst);
    inst->__impl = impl;
    inst->__size = 0;
    inst->__flags = flags;
    inst->__flags |= impl->__inst_default_flags;
  }
  if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    Xen_INSTANCE_MAPPED* mapped = (Xen_INSTANCE_MAPPED*)inst;
    mapped->__map = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
    if (!mapped->__map) {
      free(inst);
      return NULL;
    }
  }
  if (impl->__create) {
    Xen_Instance* ret = impl->__create(0, inst, args, kwargs);
    if (!ret) {
      if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
        Xen_DEL_REF(((Xen_INSTANCE_MAPPED*)inst)->__map);
      }
      free(inst);
      return NULL;
    }
    Xen_DEL_REF(ret);
  }
  return inst;
}

void __instance_free(struct __Instance* inst) {
  if (!inst) {
    return;
  }
  if (!XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_STATIC)) {
    if (inst->__impl->__destroy)
      inst->__impl->__destroy(0, inst, NULL, NULL);
    if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
      Xen_DEL_REF(((Xen_INSTANCE_MAPPED*)inst)->__map);
    }
    free(inst);
  }
}
