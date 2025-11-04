#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "xen_function_implement.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_string_implement.h"
#include "xen_vector.h"

Xen_Instance* Xen_Basic_Get_Attr_Static(ctx_id_t id, Xen_Instance* self,
                                        Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Vector_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_Instance* attr = Xen_Map_Get(Xen_IMPL(self)->__props, key);
  if (!attr) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_DEL_REF(key);
  if (Xen_IMPL(attr) == &Xen_Function_Implement) {
    Xen_Instance* method = Xen_Method_New(attr, self);
    if (!method) {
      Xen_DEL_REF(attr);
      return NULL;
    }
    Xen_DEL_REF(attr);
    return method;
  }
  return attr;
}

Xen_Instance* Xen_Basic_Set_Attr_Static(ctx_id_t id, Xen_Instance* self,
                                        Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_IMPL(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_IMPL(self)->__props) ||
      Xen_IMPL(Xen_IMPL(self)->__props) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* key = Xen_Vector_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_Instance* value = Xen_Vector_Get_Index(args, 1);
  if (!Xen_Map_Push_Pair(Xen_IMPL(self)->__props, (Xen_Map_Pair){key, value})) {
    Xen_DEL_REF(value);
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(key);
  return nil;
}
