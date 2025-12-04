#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "source_file.h"
#include "vm_def.h"
#include "xen_typedefs.h"

#define VM_CHECK_ID(id) ((run_ctx_id(Xen_VM_Current_Ctx())) == (id))

Xen_Instance* Xen_VM_Current_Ctx(void);
bool Xen_VM_Store_Global(const char*, Xen_Instance*);
bool Xen_VM_Store_Native_Function(Xen_Instance*, const char*, Xen_Native_Func,
                                  Xen_Instance*);
Xen_Instance* Xen_VM_Call_Native_Function(Xen_Native_Func, Xen_INSTANCE*,
                                          Xen_Instance*, Xen_Instance*);
Xen_INSTANCE* Xen_VM_Load_Instance(const char*, ctx_id_t);
void Xen_VM_Ctx_Clear(RunContext_ptr);
int Xen_VM_New_Ctx_Callable(CALLABLE_ptr, Xen_Instance*, struct __Instance*,
                            Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_VM_Call_Callable(CALLABLE_ptr, struct __Instance*,
                                   Xen_Instance*, Xen_Instance*, Xen_Instance*);
inline static Xen_Instance* Xen_VM_Except(void) {
  return vm->except.except;
}

inline static Xen_bool_t Xen_VM_Except_Active(void) {
  return vm->except.active;
}

void Xen_VM_Except_Show(Xen_Source_Address*, Xen_size_t);
int Xen_VM_Except_Throw(Xen_Instance*);

#endif
