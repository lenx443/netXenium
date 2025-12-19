#ifndef __XEN_FUNCTION_H__
#define __XEN_FUNCTION_H__

#include "callable.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef enum {
  XEN_FUNCTION_ARG_KIND_END,
  XEN_FUNCTION_ARG_KIND_POSITIONAL,
  XEN_FUNCTION_ARG_KIND_KEYWORD,
} Xen_Function_ArgKind;

typedef enum {
  XEN_FUNCTION_ARG_IMPL_ANY,
  XEN_FUNCTION_ARG_IMPL_BOOLEAN,
  XEN_FUNCTION_ARG_IMPL_STRING,
  XEN_FUNCTION_ARG_IMPL_NUMBER,
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
  Xen_Function_ArgSpec* spec;
} Xen_Function_ArgBinding;

Xen_Function_ArgBinding* Xen_Function_ArgsParse(Xen_Instance*, Xen_Instance*,
                                                Xen_Function_ArgSpec*);
void Xen_Function_ArgBinding_Free(Xen_Function_ArgBinding*);
Xen_Function_ArgBound* Xen_Function_ArgBinding_Search(Xen_Function_ArgBinding*,
                                                      Xen_c_string_t);
Xen_bool_t Xen_Function_ArgEmpy(Xen_Instance*, Xen_Instance*);

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func, Xen_Instance*);
Xen_INSTANCE* Xen_Function_From_Callable(CALLABLE_ptr, Xen_Instance*,
                                         Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Function_Call(Xen_Instance*, Xen_Instance*, Xen_Instance*);

#endif
