#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_tuple_implement.h"
#include "xen_tuple_instance.h"
#include "xen_tuple_iterator.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static void tuple_trace(Xen_GCHeader* h) {
  Xen_Tuple* tuple = (Xen_Tuple*)h;
  for (size_t i = 0; i < Xen_SIZE(tuple); i++) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)tuple->instances[i]);
  }
}

static Xen_Instance* tuple_alloc(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple* tuple = (Xen_Tuple*)Xen_Instance_Alloc(&Xen_Tuple_Implement);
  if (!tuple) {
    return NULL;
  }
  tuple->instances = NULL;
  return (Xen_Instance*)tuple;
}

static Xen_Instance* tuple_destroy(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple* tuple = (Xen_Tuple*)self;
  Xen_Dealloc(tuple->instances);
  return nil;
}

static Xen_Instance* tuple_string(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* self_id = Xen_Number_From_Pointer(self);
  if (!self_id) {
    return NULL;
  }
  Xen_Instance* stack = NULL;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    stack = Xen_Attr_Index_Size_Get(args, 0);
    if (Xen_IMPL(stack) != &Xen_Map_Implement) {
      return NULL;
    }
  }
  Xen_size_t roots = 0;
  if (!stack) {
    stack = Xen_Map_New();
    if (!stack) {
      return NULL;
    }
    roots++;
  } else {
    if (Xen_Map_Has(stack, self_id)) {
      Xen_Instance* string = Xen_String_From_CString("<Tuple(...)>");
      if (!string) {
        Xen_GC_Pop_Roots(roots);
        return NULL;
      }
      Xen_GC_Pop_Roots(roots);
      return string;
    }
  }
  if (!Xen_Map_Push_Pair(stack, (Xen_Map_Pair){self_id, nil})) {
    Xen_GC_Pop_Roots(roots);
    return NULL;
  }
  Xen_Tuple* tuple = (Xen_Tuple*)self;
  char* buffer = Xen_CString_Dup("<Tuple(");
  if (!buffer) {
    Xen_GC_Pop_Roots(roots);
    return NULL;
  }
  Xen_size_t buflen = 8;
  for (Xen_size_t i = 0; i < Xen_SIZE(tuple); i++) {
    Xen_Instance* value_inst = Xen_Tuple_Peek_Index(self, i);
    Xen_Instance* value_string = Xen_Attr_Raw_Stack(value_inst, stack);
    if (!value_string) {
      Xen_GC_Pop_Roots(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = Xen_CString_Dup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_GC_Pop_Roots(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buflen += Xen_CString_Len(value);
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_GC_Pop_Roots(roots);
      Xen_Dealloc((void*)value);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buffer = temp;
    strcat(buffer, value);
    Xen_Dealloc((void*)value);
    if (i != Xen_SIZE(tuple) - 1) {
      buflen += 2;
      char* tem = Xen_Realloc(buffer, buflen);
      if (!tem) {
        Xen_GC_Pop_Roots(roots);
        Xen_Dealloc(buffer);
        return NULL;
      }
      buffer = tem;
      strcat(buffer, ", ");
    }
  }
  Xen_GC_Pop_Roots(roots);
  buflen += 2;
  char* temp = Xen_Realloc(buffer, buflen);
  if (!temp) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  buffer = temp;
  strcat(buffer, ")>");
  buffer[buflen - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  if (!string) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return string;
  return string;
}

static Xen_Instance* tuple_opr_get_index(ctx_id_t id, Xen_Instance* self,
                                         Xen_Instance* args,
                                         Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != &Xen_Number_Implement) {
    return NULL;
  }
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  return ((Xen_Tuple*)self)->instances[index];
}

static Xen_Instance* tuple_iter(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Tuple_Iterator_New(self);
}

Xen_Implement Xen_Tuple_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "tuple",
    .__inst_size = sizeof(struct Xen_Tuple_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = tuple_trace,
    .__props = NULL,
    .__alloc = tuple_alloc,
    .__create = NULL,
    .__destroy = tuple_destroy,
    .__string = tuple_string,
    .__raw = tuple_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Tuple_Init() {
  if (!Xen_VM_Store_Global("tuple", (Xen_Instance*)&Xen_Tuple_Implement)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__get_index", tuple_opr_get_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__iter", tuple_iter, nil)) {
    return 0;
  }
  Xen_Tuple_Implement.__props = props;
  Xen_GC_Push_Root((Xen_GCHeader*)props);
  return 1;
}

void Xen_Tuple_Finish() {
  Xen_GC_Pop_Root();
}
