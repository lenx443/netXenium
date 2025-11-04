#ifndef __IMPLEMENT_H__
#define __IMPLEMENT_H__

#include <stddef.h>

#include "callable.h"
#include "instance.h"

struct __Instance;

#define IMPL_ALLOC(inst, args)                                                 \
  ((Xen_Instance*)inst)                                                        \
      ->__impl->__alloc(0, (Xen_Instance*)inst, (Xen_Instance*)args);
#define IMPL_DESTROY(inst, args)                                               \
  ((Xen_Instance*)inst)                                                        \
      ->__impl->__destroy(0, (Xen_Instance*)inst, (Xen_Instance*)args)
#define IMPL_STRING(inst, args)                                                \
  vm_call_native_function(((Xen_Instance*)inst)->__impl->__string,             \
                          (Xen_Instance*)inst, (Xen_Instance*)args)
#define IMPL_RAW(inst, args)                                                   \
  vm_call_native_function(((Xen_Instance*)inst)->__impl->__raw,                \
                          (Xen_Instance*)inst, (Xen_Instance*)args)
#define IMPL_CALLABLE(inst, args)                                              \
  vm_call_native_function(((Xen_Instance*)inst)->__impl->__callable,           \
                          (Xen_Instance*)inst, (Xen_Instance*)args)
#define IMPL_HASH(inst, args)                                                  \
  vm_call_native_function(((Xen_Instance*)inst)->__impl->__hash,               \
                          (Xen_Instance*)inst, (Xen_Instance*)args)

struct __Implement {
  Xen_INSTANCE_HEAD;
  char* __impl_name;
  size_t __inst_size;
  Xen_Instance_Flag __inst_default_flags;
  struct __Instance* __props;
  Xen_Native_Func __alloc;
  Xen_Native_Func __destroy;
  Xen_Native_Func __string;
  Xen_Native_Func __raw;
  Xen_Native_Func __callable;
  Xen_Native_Func __hash;
  Xen_Native_Func __get_attr;
  Xen_Native_Func __set_attr;
};

typedef struct __Implement Xen_Implement;
#define Xen_ImplementDef extern struct __Implement

#endif
