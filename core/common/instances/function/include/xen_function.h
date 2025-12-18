#ifndef __XEN_FUNCTION_H__
#define __XEN_FUNCTION_H__

#include "callable.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef enum {
  XEN_FUNCTION_ARG_KIND_END,
  XEN_FUNCTION_ARG_KIND_POSITIONAL,
  XEN_FUNCTION_ARG_KIND_KEYWORD,
  XEN_FUNCTION_ARG_KIND_OPTIONAL,
} Xen_Function_ArgKind;

typedef enum {
  XEN_FUNCTION_ARG_IMPL_ANY,
} Xen_Function_ArgImpl;

typedef struct {
  Xen_c_string_t name;
  Xen_Function_ArgKind kind;
  Xen_Function_ArgImpl impl;
  Xen_bool_t required;
  Xen_Instance* default_value;
} Xen_Function_ArgSpec;

typedef struct {
  Xen_Instance* value;
  Xen_bool_t provided;
} Xen_Function_ArgBound;

typedef struct {
  Xen_Function_ArgBound* args;
  Xen_size_t count;
} Xen_Function_ArgBinding;

Xen_Function_ArgBinding* Xen_Function_ArgsParse(Xen_Instance*, Xen_Instance*,
                                                Xen_Function_ArgSpec*);

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func, Xen_Instance*);
Xen_INSTANCE* Xen_Function_From_Callable(CALLABLE_ptr, Xen_Instance*,
                                         Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Function_Call(Xen_Instance*, Xen_Instance*, Xen_Instance*);

#endif
