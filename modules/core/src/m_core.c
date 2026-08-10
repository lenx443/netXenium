#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "attrs.h"
#include "callable.h"
#include "instance.h"
#include "m_core.h"
#include "program.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_life.h"
#include "xen_module.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

static Xen_Instance* fn_exit(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* exit_code = Xen_Tuple_Get_Index(args, 0);
    if (Xen_IMPL(exit_code) != xen_globals->implements->number) {
      return NULL;
    }
    xen_globals->program->closed = 1;
    xen_globals->program->exit_code = Xen_Number_As_Int(exit_code);
    return nil;
  }
  xen_globals->program->closed = 1;
  xen_globals->program->exit_code = 0;
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
    if (Xen_IMPL(string) != xen_globals->implements->string) {
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    return nil;
  }
  Xen_Instance* out_reg = xen_register_prop_get("__out");
  if (!out_reg) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_String(out_reg);
  if (!string) {
    return NULL;
  }
  if (Xen_IMPL(string) != xen_globals->implements->string) {
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
    if (Xen_IMPL(string) != xen_globals->implements->string) {
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
    if (Xen_IMPL(string) != xen_globals->implements->string) {
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
  if (Xen_IMPL(inst) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_c_string_t mod_name = Xen_String_As_CString(inst);
  return Xen_Load(mod_name);
}

static Xen_Instance* fn_check_register(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(inst) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_Instance* result = xen_register_prop_get(Xen_String_As_CString(inst));
  if (result == NULL) {
    return Xen_False;
  }
  return Xen_True;
}

static Xen_Instance* fn_get_props(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  if (XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_MAPPED)) {
    return (Xen_Instance*)((Xen_Instance_Mapped*)inst)->__map->ptr;
  }
  return nil;
}

static Xen_Instance* fn_has_attr(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* attr = Xen_Tuple_Get_Index(args, 1);
  Xen_Instance* result = Xen_Attr_Get(inst, attr);
  if (!result) {
    return Xen_False;
  }
  return Xen_True;
}

static Xen_Instance* init(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Attr_Set_Str(self, "boolean", (Xen_Instance*)xen_globals->implements->boolean);
  Xen_Attr_Set_Str(self, "bytes", (Xen_Instance*)xen_globals->implements->bytes);
  Xen_Attr_Set_Str(self, "except", (Xen_Instance*)xen_globals->implements->except);
  Xen_Attr_Set_Str(self, "map", (Xen_Instance*)xen_globals->implements->map);
  Xen_Attr_Set_Str(self, "method", (Xen_Instance*)xen_globals->implements->method);
  Xen_Attr_Set_Str(self, "number", (Xen_Instance*)xen_globals->implements->number);
  Xen_Attr_Set_Str(self, "queue", (Xen_Instance*)xen_globals->implements->queue);
  Xen_Attr_Set_Str(self, "string", (Xen_Instance*)xen_globals->implements->string);
  Xen_Attr_Set_Str(self, "tuple", (Xen_Instance*)xen_globals->implements->tuple);
  Xen_Attr_Set_Str(self, "vector", (Xen_Instance*)xen_globals->implements->vector);

  Xen_Attr_Set_Str(self, "true", Xen_True);
  Xen_Attr_Set_Str(self, "false", Xen_False);
  return nil;
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
    {"check_register", fn_check_register},
    {"get_props", fn_get_props},
    {"has_attr", fn_has_attr},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Core = {
    .mod_name = "core",
    .mod_init = init,
    .mod_functions = core_functions,
    .mod_functions_async = NULL,
    .mod_implements = NULL,
};
