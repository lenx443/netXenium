#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "attrs.h"
#include "callable.h"
#include "instance.h"
#include "m_core.h"
#include "run_ctx.h"
#include "xen_alloc.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* fn_echo(ctx_id_t id, Xen_Instance* self,
                             Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  }
  if (Xen_SIZE(args) == 1) {
    Xen_Instance* inst = Xen_Attr_Index_Size_Get(args, 0);
    if (!inst) {
      return NULL;
    }
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_DEL_REF(inst);
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      Xen_DEL_REF(string);
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    Xen_DEL_REF(string);
    return nil;
  }
  Xen_Instance* out_reg = xen_register_prop_get("__out", id);
  if (!out_reg) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_String(out_reg);
  if (!string) {
    Xen_DEL_REF(out_reg);
    return NULL;
  }
  Xen_DEL_REF(out_reg);
  if (Xen_IMPL(string) != &Xen_String_Implement) {
    Xen_DEL_REF(string);
    return NULL;
  }
  fputs(Xen_String_As_CString(string), stdout);
  Xen_DEL_REF(string);
  fputs(Xen_String_As_CString(out_reg), stdout);
  return nil;
}

static Xen_Instance* fn_print(ctx_id_t id, Xen_Instance* self,
                              Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* inst = Xen_Attr_Index_Size_Get(args, i);
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_DEL_REF(inst);
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      Xen_DEL_REF(string);
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    Xen_DEL_REF(string);
  }
  return nil;
}

static Xen_Instance* fn_println(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* inst = Xen_Attr_Index_Size_Get(args, i);
    Xen_Instance* string = Xen_Attr_String(inst);
    if (!string || Xen_Nil_Eval(string)) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_DEL_REF(inst);
    if (Xen_IMPL(string) != &Xen_String_Implement) {
      Xen_DEL_REF(string);
      return NULL;
    }
    fputs(Xen_String_As_CString(string), stdout);
    Xen_DEL_REF(string);
  }
  fputc('\n', stdout);
  return nil;
}

static Xen_Instance* fn_readline(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
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

static Xen_Instance* fn_size(ctx_id_t id, Xen_Instance* self,
                             Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* inst = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* size = Xen_Number_From_Int64(Xen_SIZE(inst));
  if (!size) {
    Xen_DEL_REF(inst);
    return NULL;
  }
  Xen_DEL_REF(inst);
  return size;
}

static Xen_Instance* fn_test_vector(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* v1 = Xen_Number_From_Int64(1);
  Xen_Instance* v2 = Xen_Number_From_Int64(2);
  Xen_Instance* v3 = Xen_Number_From_Int64(3);
  Xen_Instance* v4 = Xen_Number_From_Int64(4);
  Xen_Instance* v5 = Xen_Number_From_Int64(5);
  Xen_Instance* vec =
      Xen_Vector_From_Array(5, (Xen_Instance*[]){v1, v2, v3, v4, v5});
  Xen_DEL_REF(v5);
  Xen_DEL_REF(v4);
  Xen_DEL_REF(v3);
  Xen_DEL_REF(v2);
  Xen_DEL_REF(v1);
  return vec;
}

static Xen_Module_Function_Table core_functions = {
    {"echo", fn_echo},
    {"print", fn_print},
    {"println", fn_println},
    {"readline", fn_readline},
    {"size", fn_size},
    {"test_vector", fn_test_vector},
    {NULL, NULL},
};

struct Xen_Module_Def Module_Core = {
    .mod_name = "core",
    .mod_init = NULL,
    .mod_functions = core_functions,
};
