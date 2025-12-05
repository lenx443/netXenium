#ifndef __XEN_EXCEPT_H__
#define __XEN_EXCEPT_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Except_New(Xen_c_string_t, Xen_c_string_t);
Xen_Instance* Xen_Except_New_CFormat(Xen_c_string_t, Xen_c_string_t, ...);

#define Xen_SyntaxError(msg)                                                   \
  Xen_VM_Except_Throw(Xen_Except_New("SyntaxError", msg))

#define Xen_SyntaxError_Format(msg, ...)                                       \
  Xen_VM_Except_Throw(Xen_Except_New_CFormat("SyntaxError", msg, ##__VA_ARGS__))

#endif
