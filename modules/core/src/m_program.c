#include "callable.h"
#include "instance.h"
#include "m_program.h"
#include "xen_boolean.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"

static Xen_Instance *fn_exist_prev(Xen_Instance *self, Xen_Instance *args,
                                   Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (xen_globals->program->prev_program == NULL) { return Xen_False; }
  return Xen_True;
}

static Xen_Instance *fn_get_prev_register(Xen_Instance *self, Xen_Instance *args,
                                          Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Instance *rg_name = Xen_Tuple_Get_Index(args, 0);
  if (!rg_name || !Xen_IsString(rg_name)) { return NULL; }
  if (xen_globals->program->prev_program == NULL) { return NULL; }
  Xen_Instance *result = Xen_Map_Get(
      (Xen_Instance *)xen_globals->program->prev_program->vm->globals_props->ptr,
      rg_name);
  return result;
}

static Xen_Instance *fn_set_prev_register(Xen_Instance *self, Xen_Instance *args,
                                          Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Instance *rg_name = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance *rg_value = Xen_Tuple_Get_Index(args, 1);
  if (!rg_name || !rg_value || !Xen_IsString(rg_name)) { return NULL; }
  if (xen_globals->program->prev_program == NULL) { return NULL; }
  Xen_Map_Push_Pair(
      (Xen_Instance *)xen_globals->program->prev_program->vm->globals_props->ptr,
      (Xen_Map_Pair){rg_name, rg_value});
  return nil;
}

static Xen_Module_Function_Table functions = {
    {"exist_prev", fn_exist_prev},
    {"get_prev_register", fn_get_prev_register},
    {"set_prev_register", fn_set_prev_register},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Program = {
    "program", NULL, functions, NULL, NULL,
};
