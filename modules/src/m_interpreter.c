#include "m_interpreter.h"
#include "callable.h"
#include "compiler.h"
#include "instance.h"
#include "interpreter.h"
#include "run_ctx.h"
#include "xen_igc.h"
#include "xen_module_types.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

static Xen_Instance* fn_interpreter(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_size_t roots = 0;
  Xen_Instance* code = Xen_Tuple_Get_Index(args, 0);
  if (!code) {
    return NULL;
  }
  if (Xen_IMPL(code) != &Xen_String_Implement) {
    return NULL;
  }
  Xen_IGC_XPUSH(code, roots);
  Xen_uint8_t mode = Xen_COMPILE_PROGRAM;
  if (Xen_SIZE(args) == 2) {
    Xen_Instance* mode_inst = Xen_Tuple_Get_Index(args, 1);
    mode = Xen_Number_As_Int(mode_inst);
  }
  Xen_IGC_XPOP(roots);
  return interpreter(Xen_String_As_CString(code), mode);
}

static Xen_Module_Function_Table interpreter_functions = {
    {"interpreter", fn_interpreter},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Interpreter = {
    .mod_name = "interpreter",
    .mod_init = NULL,
    .mod_functions = interpreter_functions,
};
