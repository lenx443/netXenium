#include "m_interpreter.h"
#include "callable.h"
#include "compiler.h"
#include "instance.h"
#include "interpreter.h"
#include "netxenium/attrs.h"
#include "netxenium/xen_nil.h"
#include "xen_function.h"
#include "parser.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_module_types.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

static Xen_Instance* fn_interpreter(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Function_ArgSpec args_def[] = {
      {"file_name", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"file_content", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"compile_mode", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {"globals", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY, XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {"instances", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY, XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_c_string_t file_name = Xen_String_As_CString(
    Xen_Function_ArgBinding_Search(binding, "file_name")->value
  );
  Xen_c_string_t file_content = Xen_String_As_CString(
    Xen_Function_ArgBinding_Search(binding, "file_content")->value
  );
  Xen_Function_ArgBound* compile_mode_arg =
      Xen_Function_ArgBinding_Search(binding, "compile_mode");
  int compile_mode = Xen_COMPILE_PROGRAM;
  if (compile_mode_arg->provided) {
    compile_mode = Xen_Number_As_Int(compile_mode_arg->value);
  }
  Xen_Instance *globals = Xen_Function_ArgBinding_Search(binding, "globals")->value;
  if (globals && Xen_IMPL(globals) != xen_globals->implements->map) {
    return NULL;
  }
  Xen_Instance *instances = Xen_Function_ArgBinding_Search(binding, "instances")->value;
  if (instances && Xen_IMPL(instances) != xen_globals->implements->map) {
    return NULL;
  }
  Xen_Function_ArgBinding_Free(binding);
  return interpreter(file_name, file_content, compile_mode, globals, instances, NULL);
}

static Xen_Instance* fn_eval(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* code = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(code) != xen_globals->implements->string) {
    return NULL;
  }
  return Xen_Eval(Xen_String_As_CString(code));
}

static Xen_Instance* fn_parse(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_size_t roots = 0;
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* name = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(name) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_XPUSH(name, roots);
  Xen_Instance* code = Xen_Tuple_Get_Index(args, 1);
  if (Xen_IMPL(code) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_XPUSH(code, roots);
  return Xen_Parser(Xen_String_As_CString(name), Xen_String_As_CString(code),
                    Xen_SIZE(code));
}

static Xen_Module_Function_Table interpreter_functions = {
    {"interpreter", fn_interpreter},
    {"eval", fn_eval},
    {"parse", fn_parse},
    {NULL, NULL},
};

static Xen_Instance* init(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Attr_Set_Str(self, "COMPILE_PROGRAM", Xen_Number_From_Int(Xen_COMPILE_PROGRAM));
  Xen_Attr_Set_Str(self, "COMPILE_REPL", Xen_Number_From_Int(Xen_COMPILE_PROGRAM));
  Xen_Attr_Set_Str(self, "COMPILE_FUNCTION", Xen_Number_From_Int(Xen_COMPILE_FUNCTION));
  Xen_Attr_Set_Str(self, "COMPILE_FUNCTION_ASYNC", Xen_Number_From_Int(Xen_COMPILE_FUNCTION_ASYNC));
  Xen_Attr_Set_Str(self, "COMPILE_IMPLEMENT", Xen_Number_From_Int(Xen_COMPILE_IMPLEMENT));
  Xen_Attr_Set_Str(self, "COMPILE_EXPR", Xen_Number_From_Int(Xen_COMPILE_EXPR));
  return nil;
}

struct Xen_Module_Def Module_Interpreter = {
    .mod_name = "interpreter",
    .mod_init = init,
    .mod_functions = interpreter_functions,
    .mod_implements = NULL,
};
