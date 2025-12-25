#ifndef __NETXENIUM_H__
#define __NETXENIUM_H__

#include "attrs.h"
#include "basic_templates.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_bytes.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_function.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

#define Xen_NULL ((void*)0)

struct Xen_Module_Def;

#ifdef XEN_ABI
struct Xen_Module_Function;
#else
struct Xen_Module_Function {
  char* fun_name;
  Xen_Native_Func fun_func;
};
#endif

extern struct __Implement Xen_Basic;

#ifndef XEN_ABI
Xen_size_t Xen_SIZE(void* inst);
#endif

struct Xen_Module_Def* Xen_Module_Define(Xen_c_string_t, Xen_Native_Func,
                                         struct Xen_Module_Function*,
                                         Xen_ImplementStruct**);
void Xen_Debug_Print(Xen_c_string_t, ...);
void Xen_GetReady(void*);

#endif
