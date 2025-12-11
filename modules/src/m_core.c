#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "attrs.h"
#include "callable.h"
#include "instance.h"
#include "m_core.h"
#include "program.h"
#include "run_ctx_instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_module.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* fn_exit(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* exit_code = Xen_Tuple_Get_Index(args, 0);
    if (Xen_IMPL(exit_code) != &Xen_Number_Implement) {
      return NULL;
    }
    program.closed = 1;
    program.exit_code = Xen_Number_As_Int(exit_code);
    return nil;
  }
  program.closed = 1;
  program.exit_code = 0;
  return nil;
}

static Xen_Instance* fn_echo(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  }
  if (Xen_SIZE(args) == 1) {
    Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
    if (!inst) {
      return NULL;
    }
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string) {
      return NULL;
    }
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    return nil;
  }
  Xen_Instance* out_reg = xen_register_prop_get(
      "__out", ((RunContext_ptr)Xen_VM_Current_Ctx())->ctx_id);
  if (!out_reg) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_String(out_reg);
  if (!string) {
    return NULL;
  }
  if (Xen_IMPL(string) != &Xen_String_Implement) {
    return NULL;
  }
  fputs(Xen_String_As_CString(string), stdout);
  return nil;
}

static Xen_Instance* fn_print(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* inst = Xen_Tuple_Get_Index(args, i);
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string) {
      return NULL;
    }
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
  }
  return nil;
}

static Xen_Instance* fn_println(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* inst = Xen_Tuple_Get_Index(args, i);
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string || Xen_Nil_Eval(string)) {
      return NULL;
    }
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
  }
  fputc('\n', stdout);
  return nil;
}

static Xen_Instance* fn_readline(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* buffer = NULL;
  Xen_size_t bufsiz = 0;
  char c = '\0';
  while (c != '\n') {
    c = fgetc(stdin);
    if (c == (char)EOF) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    char* temp = Xen_Realloc(buffer, bufsiz + 1);
    if (!temp) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    buffer = temp;
    buffer[bufsiz++] = c;
  }
  buffer[strcspn(buffer, "\n")] = '\0';
  Xen_Instance* rsult = Xen_String_From_CString(buffer);
  if (!rsult) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return rsult;
}

static Xen_Instance* fn_size(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* size = Xen_Number_From_Int64(Xen_SIZE(inst));
  if (!size) {
    return NULL;
  }
  return size;
}

static Xen_Instance* fn_id(Xen_Instance* self, Xen_Instance* args,
                           Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* r_id = Xen_Number_From_Pointer(inst);
  if (!r_id) {
    return NULL;
  }
  return r_id;
}

static Xen_Instance* fn_impl(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  return (Xen_Instance*)Xen_IMPL(inst);
}

static Xen_Instance* fn_load(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(inst) != &Xen_String_Implement) {
    return NULL;
  }
  Xen_c_string_t mod_name = Xen_String_As_CString(inst);
  if (Xen_SIZE(vm->modules_stack) > 0) {
    Xen_Module* mod_top = (Xen_Module*)Xen_Vector_Top(vm->modules_stack);
    Xen_c_string_t path = mod_top->mod_path;
    Xen_ssize_t psize = snprintf(NULL, 0, "%s/%s.nxm", path, mod_name);
    if (psize == -1) {
      return NULL;
    }
    Xen_string_t relative_path = Xen_Alloc(psize + 1);
    snprintf(relative_path, psize + 1, "%s/%s.nxm", path, mod_name);
    Xen_Instance* mod = Xen_Module_Load(relative_path, mod_name, path);
    if (!mod) {
      Xen_Dealloc((void*)relative_path);
      return NULL;
    }
    Xen_Dealloc((void*)relative_path);
    return mod;
  } else {
    char path[1024];
    if (!getcwd(path, 1024)) {
      return NULL;
    }
    Xen_ssize_t psize = snprintf(NULL, 0, "%s/%s.nxm", path, mod_name);
    if (psize == -1) {
      return NULL;
    }
    Xen_string_t relative_path = Xen_Alloc(psize + 1);
    snprintf(relative_path, psize + 1, "%s/%s.nxm", path, mod_name);
    Xen_Instance* mod = Xen_Module_Load(relative_path, mod_name, path);
    if (!mod) {
      Xen_Dealloc((void*)relative_path);
      return NULL;
    }
    Xen_Dealloc((void*)relative_path);
    return mod;
  }
}

static Xen_Module_Function_Table core_functions = {
    {"exit", fn_exit},
    {"echo", fn_echo},
    {"print", fn_print},
    {"println", fn_println},
    {"readline", fn_readline},
    {"size", fn_size},
    {"id", fn_id},
    {"impl", fn_impl},
    {"load", fn_load},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Core = {
    .mod_name = "core",
    .mod_init = NULL,
    .mod_functions = core_functions,
};
