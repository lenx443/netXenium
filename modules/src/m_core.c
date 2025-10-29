#include <stdio.h>

#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "m_core.h"
#include "operators.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_map.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_typedefs.h"

static Xen_Instance* fn_echo(ctx_id_t id, Xen_Instance* self,
                             Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  }
  if (Xen_SIZE(args) == 1) {
    Xen_Instance* inst = Xen_Operator_Eval_Pair_Steal2(
        args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);
    if (!inst || !Xen_TYPE(inst)->__string) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_Instance* string =
        vm_call_native_function(Xen_TYPE(inst)->__string, inst, nil);
    if (!string || Xen_Nil_Eval(string)) {
      return NULL;
    }
    Xen_DEL_REF(inst);
    if (Xen_TYPE(string) != &Xen_String_Implement) {
      Xen_DEL_REF(string);
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    Xen_DEL_REF(string);
    return nil;
  }
  Xen_Instance* out_reg = xen_register_prop_get("__out", id);
  if (!Xen_TYPE(out_reg)->__string) {
    Xen_DEL_REF(out_reg);
    return NULL;
  }
  Xen_Instance* string =
      vm_call_native_function(Xen_TYPE(out_reg)->__string, out_reg, nil);
  if (!string || Xen_Nil_Eval(string)) {
    return NULL;
  }
  Xen_DEL_REF(out_reg);
  if (Xen_TYPE(string) != &Xen_String_Implement) {
    Xen_DEL_REF(string);
    return NULL;
  }
  fputs(Xen_String_As_CString(string), stdout);
  Xen_DEL_REF(string);
  fputs(Xen_String_As_CString(out_reg), stdout);
  return nil;
}

static Xen_Instance* fn_fun(ctx_id_t id, Xen_Instance* self,
                            Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* inst = Xen_Operator_Eval_Pair_Steal2(
        args, Xen_Number_From_Int64(i), Xen_OPR_GET_INDEX);
    if (!Xen_TYPE(inst)->__string) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_Instance* string =
        vm_call_native_function(Xen_TYPE(inst)->__string, inst, nil);
    if (!string || Xen_Nil_Eval(string)) {
      return NULL;
    }
    Xen_DEL_REF(inst);
    if (Xen_TYPE(string) != &Xen_String_Implement) {
      Xen_DEL_REF(string);
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    Xen_DEL_REF(string);
  }
  return Xen_Map_From_Pairs_Str_With_Size(
      5,
      (Xen_Map_Pair_Str[]){{"0", Xen_Number_From_Int(12)},
                           {"1", Xen_Number_From_Int(1)},
                           {"2", Xen_Number_From_Int(6)},
                           {"3", Xen_Number_From_Int(5)},
                           {"0", Xen_Number_From_Int(10)}},
      XEN_MAP_DEFAULT_CAP);
}

static Xen_Module_Function_Table core_functions = {
    {"echo", fn_echo},
    {"fun", fn_fun},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Core = {
    .mod_name = "core",
    .mod_init = NULL,
    .mod_functions = core_functions,
};
