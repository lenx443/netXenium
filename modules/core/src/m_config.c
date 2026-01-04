#include "m_config.h"
#include "callable.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_life.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_vector.h"

static Xen_Instance* fn_add_absolute_path(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* path = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(path) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_Vector_Push((Xen_Instance*)(*xen_globals->vm)->paths_modules->ptr, path);
  return nil;
}

static Xen_Instance* fn_add_relative_path(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* path = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(path) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_c_string_t current_path = NULL;
  if (Xen_SIZE((*xen_globals->vm)->modules_stack) > 0) {
    Xen_Module* mod_top = (Xen_Module*)Xen_Vector_Top(
        (Xen_Instance*)(*xen_globals->vm)->modules_stack->ptr);
    current_path = mod_top->mod_path;
  } else {
    current_path = (*xen_globals->vm)->path_current;
  }
  Xen_c_string_t path_str = Xen_String_As_CString(path);
  Xen_ssize_t psize = snprintf(NULL, 0, "%s/%s", current_path, path_str);
  if (psize == -1) {
    return NULL;
  }
  Xen_string_t full_path = Xen_Alloc(psize + 1);
  snprintf(full_path, psize + 1, "%s/%s", current_path, path_str);
  Xen_Instance* abs_path = Xen_String_From_CString(full_path);
  Xen_Vector_Push((Xen_Instance*)(*xen_globals->vm)->paths_modules->ptr,
                  abs_path);
  return nil;
}

static Xen_Module_Function_Table functions = {
    {"add_absolute_path", fn_add_absolute_path},
    {"add_relative_path", fn_add_relative_path},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Config = {
    .mod_name = "config",
    .mod_init = NULL,
    .mod_functions = functions,
    .mod_implements = NULL,
};
