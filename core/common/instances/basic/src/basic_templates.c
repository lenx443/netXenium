#include <string.h>

#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_function_implement.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

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
      Xen_IMPL(Xen_IMPL(self)->__props) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
    return NULL;
  }
  Xen_IGC_XPUSH(key, roots);
  Xen_Instance* attr = Xen_Map_Get(Xen_IMPL(self)->__props, key);
  if (!attr) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPUSH(attr, roots);
  if (Xen_IMPL(attr) == &Xen_Function_Implement) {
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
      Xen_IMPL(Xen_IMPL(self)->__props) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
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
