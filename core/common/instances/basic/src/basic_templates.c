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
  if (Xen_TYPE(self)->__props == NULL ||
      Xen_Nil_Eval(Xen_TYPE(self)->__props) ||
      Xen_TYPE(Xen_TYPE(self)->__props) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Vector_Get_Index(args, 0);
  if (Xen_TYPE(key) != &Xen_String_Implement) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_Instance* attr = Xen_Map_Get(Xen_TYPE(self)->__props, key);
  if (!attr) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_DEL_REF(key);
  if (Xen_TYPE(attr) == &Xen_Function_Implement) {
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
