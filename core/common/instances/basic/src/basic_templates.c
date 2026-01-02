#include <string.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Basic_New(Xen_c_string_t name, Xen_Instance* props,
                            Xen_Implement* base) {
  Xen_Implement* impl = (Xen_Implement*)__instance_new(&Xen_Basic, nil, nil, 0);
  if (!impl) {
    return NULL;
  }
  impl->__impl_name = Xen_CString_Dup(name);
  if (!impl->__impl_name) {
    return NULL;
  }
  impl->__inst_size = sizeof(Xen_Instance_Mapped);
  impl->__inst_default_flags = XEN_INSTANCE_FLAG_MAPPED;
  impl->__inst_trace = Xen_Basic_Mapped_Trace;
  if (!props) {
    props = Xen_Map_New();
    if (!props) {
      return NULL;
    }
  }
  Xen_IGC_WRITE_FIELD(impl, impl->__props, props);
  if (base) {
    Xen_IGC_WRITE_FIELD(impl, impl->__base, base);
  }
  impl->__get_attr = Xen_Basic_Get_Attr_Mapped;
  impl->__set_attr = Xen_Basic_Set_Attr_Mapped;
  printf("basic: %p\n", (void*)impl);
  return (Xen_Instance*)impl;
}

void Xen_Basic_Mapped_Trace(Xen_GCHeader* h) {
  Xen_Instance_Mapped* mapped = (Xen_Instance_Mapped*)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)mapped->__map);
}

Xen_Instance* Xen_Basic_String(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* addr = Xen_CString_From_Pointer(self);
  if (!addr) {
    return NULL;
  }
  Xen_size_t size = 1 + Xen_CString_Len(Xen_IMPL(self)->__impl_name) + 2 +
                    Xen_CString_Len(addr) + 2;
  char* buffer = Xen_Alloc(size);
  if (!buffer) {
    Xen_Dealloc(addr);
    return NULL;
  }
  buffer[0] = '<';
  buffer[1] = '\0';
  strcat(buffer, Xen_IMPL(self)->__impl_name);
  strcat(buffer, "::");
  strcat(buffer, addr);
  buffer[size - 2] = '>';
  buffer[size - 1] = '\0';
  Xen_Dealloc(addr);
  Xen_Instance* string = Xen_String_From_CString(buffer);
  if (!string) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return string;
}

Xen_Instance* Xen_Basic_Get_Attr_Static(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != xen_globals->implements->map) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_XPUSH(key, roots);
  Xen_Instance* attr = Xen_Map_Get(Xen_IMPL(self)->__props, key);
  if (!attr) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPUSH(attr, roots);
  if (Xen_IMPL(attr) == xen_globals->implements->function) {
    Xen_Instance* method = Xen_Method_New(attr, self);
    if (!method) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    Xen_IGC_XPOP(roots);
    return method;
  }
  Xen_IGC_XPOP(roots);
  return attr;
}

Xen_Instance* Xen_Basic_Set_Attr_Static(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != xen_globals->implements->map) {
    return NULL;
  }
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_Push(key);
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 1);
  if (!Xen_Map_Push_Pair(Xen_IMPL(self)->__props, (Xen_Map_Pair){key, value})) {
    return NULL;
  }
  Xen_IGC_Pop();
  return nil;
}

Xen_Instance* Xen_Basic_Get_Attr_Mapped(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != xen_globals->implements->map) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance_Mapped* mapped = (Xen_Instance_Mapped*)self;
  Xen_size_t roots = 0;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_XPUSH(key, roots);
  Xen_Instance* attr = Xen_Map_Get(mapped->__map, key);
  if (!attr) {
    attr = Xen_Map_Get(Xen_IMPL(self)->__props, key);
    if (!attr) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
  }
  Xen_IGC_XPUSH(attr, roots);
  if (Xen_IMPL(attr) == xen_globals->implements->function) {
    Xen_Instance* method = Xen_Method_New(attr, self);
    if (!method) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    Xen_IGC_XPOP(roots);
    return method;
  }
  Xen_IGC_XPOP(roots);
  return attr;
}

Xen_Instance* Xen_Basic_Set_Attr_Mapped(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != xen_globals->implements->map) {
    return NULL;
  }
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance_Mapped* mapped = (Xen_Instance_Mapped*)self;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_Push(key);
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 1);
  if (!Xen_Map_Push_Pair(mapped->__map, (Xen_Map_Pair){key, value})) {
    return NULL;
  }
  Xen_IGC_Pop();
  return nil;
}
