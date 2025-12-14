#include "xen_except_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except_instance.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

static Xen_Instance* except_alloc(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Except* except =
      (Xen_Except*)Xen_Instance_Alloc(xen_globals->implements->except);
  if (!except) {
    return NULL;
  }
  except->type = NULL;
  except->message = NULL;
  return (Xen_Instance*)except;
}

static Xen_Instance* except_create(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Except* except = (Xen_Except*)self;
  if (Xen_SIZE(args) < 1 || Xen_SIZE(args) > 2) {
    return NULL;
  }
  Xen_Instance* type = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(type) != xen_globals->implements->string) {
    return NULL;
  }
  except->type = Xen_CString_Dup(Xen_String_As_CString(type));
  Xen_Instance* message = Xen_Tuple_Get_Index(args, 1);
  if (message) {
    if (Xen_IMPL(message) != xen_globals->implements->string) {
      return NULL;
    }
    except->message = Xen_CString_Dup(Xen_String_As_CString(message));
  }
  return nil;
}

static Xen_Instance* except_destroy(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Except* except = (Xen_Except*)self;
  if (except->type) {
    Xen_Dealloc((void*)except->type);
  }
  if (except->message) {
    Xen_Dealloc((void*)except->message);
  }
  return nil;
}

static Xen_Instance* except_except(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return self;
}

static Xen_Instance* except_type(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(self)) {
    return NULL;
  }
  return Xen_String_From_CString(((Xen_Except*)self)->type);
}

static Xen_Instance* except_message(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(self)) {
    return NULL;
  }
  Xen_c_string_t message = ((Xen_Except*)self)->message;
  if (message) {
    return Xen_String_From_CString(message);
  }
  return nil;
}

Xen_Implement __Except_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_MAPPED),
    .__impl_name = "Except",
    .__inst_size = sizeof(struct Xen_Except_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = NULL,
    .__props = NULL,
    .__base = NULL,
    .__alloc = except_alloc,
    .__create = except_create,
    .__destroy = except_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

struct __Implement* Xen_Except_GetImplement(void) {
  return &__Except_Implement;
}

int Xen_Except_Init(void) {
  if (!Xen_VM_Store_Global("except",
                           (Xen_Instance*)xen_globals->implements->except)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__except", except_except, nil) ||
      !Xen_VM_Store_Native_Function(props, "type", except_type, nil) ||
      !Xen_VM_Store_Native_Function(props, "message", except_message, nil)) {
    return 0;
  }
  Xen_IGC_Fork_Push(impls_maps, props);
  __Except_Implement.__props = props;
  return 1;
}

void Xen_Except_Finish(void) {}
