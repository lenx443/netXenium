#include <stdio.h>

#include "instance.h"
#include "m_core.h"
#include "run_ctx.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_vector.h"

static int fn_echo(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  if (Xen_Vector_Size(args) > 1) { return 0; }
  if (Xen_Vector_Size(args) == 1) {
    if (Xen_TYPE(Xen_Vector_Get_Index(args, 0)) != &Xen_String_Implement) { return 0; }
    fputs(Xen_String_As_CString(Xen_Vector_Get_Index(args, 0)), stdout);
    return 1;
  }
  Xen_Instance *out_reg = xen_register_prop_get("__out", id);
  if (Xen_Nil_Eval(out_reg) || Xen_TYPE(out_reg) != &Xen_String_Implement) { return 0; }
  fputs(Xen_String_As_CString(out_reg), stdout);
  Xen_DEL_REF(out_reg);
  return 1;
}

static Xen_Module_Command_Table core_commands = {
    {"echo", fn_echo},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Core = {
    .mod_name = "core",
    .mod_init = NULL,
    .mod_commands = core_commands,
};
