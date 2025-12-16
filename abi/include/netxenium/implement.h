#ifndef __IMPLEMENT_H__
#define __IMPLEMENT_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"
struct __Instance;

#define NATIVE_CLEAR_ARG_NEVER_USE                                             \
  (void)(self);                                                                \
  (void)(args);                                                                \
  (void)(kwargs);

typedef struct __Instance* (*Xen_Native_Func)(struct __Instance*,
                                              struct __Instance*,
                                              struct __Instance*);

struct __Implement;
struct __ImplementStruct {
  char* __impl_name;
  Xen_size_t __inst_size;
  Xen_Instance_Flag __inst_default_flags;
  void (*__inst_trace)(Xen_GCHeader*);
  struct __Instance* __props;
  struct __Implement* __base;
  Xen_Native_Func __alloc;
  Xen_Native_Func __create;
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
typedef struct __ImplementStruct Xen_ImplementStruct;

void Xen_Implement_SetProps(Xen_Implement*, Xen_Instance*);

#endif
