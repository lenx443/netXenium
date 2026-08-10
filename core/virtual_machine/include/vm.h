#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "vm_def.h"
#include "xen_typedefs.h"

VM* Xen_VM(void);
Xen_Instance* Xen_VM_Current_Ctx(void);
void Xen_VM_Set_Current_Ctx(Xen_Instance*);
bool Xen_VM_Store_Native_Function(Xen_Instance*, const char*, Xen_Native_Func,
                                  Xen_Instance*);
bool Xen_VM_Store_Native_Function_Async(Xen_Instance*, const char*, Xen_Native_Func_Async, Xen_size_t);
Xen_Instance* Xen_VM_Call_Native_Function(Xen_Native_Func, Xen_INSTANCE*,
                                          Xen_Instance*, Xen_Instance*);
Xen_INSTANCE* Xen_VM_Load_Instance(const char*);
void Xen_VM_Ctx_Clear(RunContext_ptr);
inline static Xen_Instance* Xen_VM_Except(void) {
  return (Xen_Instance*)Xen_VM()->except.except->ptr;
}

inline static Xen_bool_t Xen_VM_Except_Active(void) {
  return Xen_VM()->except.active;
}

void Xen_VM_Except_Backtrace_Show(void);
int Xen_VM_Except_Throw(Xen_Instance*);

#endif
