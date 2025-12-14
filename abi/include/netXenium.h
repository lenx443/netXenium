#ifndef __NETXENIUM_H__
#define __NETXENIUM_H__

#include "implement.h"
#include "instance.h"
#include "xen_number.h"
#include "xen_typedefs.h"

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

struct Xen_Module_Def* Xen_Module_Define(Xen_c_string_t, Xen_Native_Func,
                                         struct Xen_Module_Function*,
                                         Xen_ImplementStruct**);
void Xen_Debug_Print(Xen_c_string_t, ...);
void Xen_GetReady(void*);
Xen_Implement* Xen_Implement_Make(Xen_c_string_t, Xen_size_t);

#endif
