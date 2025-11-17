#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_set.h"
#include "xen_set_implement.h"
#include "xen_string.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"
#include "xen_vector_iterator.h"

static Xen_Instance* vector_alloc(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)Xen_Instance_Alloc(&Xen_Vector_Implement);
  if (!vector) {
    return NULL;
  }
  vector->values = NULL;
  vector->capacity = 0;
  return (Xen_Instance*)vector;
}

static Xen_Instance* vector_create(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* iterable = Xen_Attr_Index_Size_Get(args, 0);
    if (self == iterable) {
      Xen_DEL_REF(iterable);
      return nil;
    }
    Xen_Instance* iter = Xen_Attr_Iter(iterable);
    if (!iter) {
      Xen_DEL_REF(iterable);
      return NULL;
    }
    Xen_DEL_REF(iterable);
    Xen_Instance* value = NULL;
    while ((value = Xen_Attr_Next(iter)) != NULL) {
      if (!Xen_Vector_Push(self, value)) {
        Xen_DEL_REF(value);
        Xen_DEL_REF(iter);
        return NULL;
      }
      Xen_DEL_REF(value);
    }
    Xen_DEL_REF(iter);
  }
  return nil;
}

static Xen_Instance* vector_destroy(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector* vector = (Xen_Vector*)self;
  for (size_t i = 0; i < vector->__size; i++) {
    Xen_DEL_REF(vector->values[i]);
  }
  Xen_Dealloc(vector->values);
  return nil;
}

static Xen_Instance* vector_string(ctx_id_t id, Xen_Instance* self,
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
    if (Xen_IMPL(stack) != &Xen_Set_Implement) {
      Xen_DEL_REF(self_id);
      Xen_DEL_REF(stack);
      return NULL;
    }
  }
  if (!stack) {
    stack = Xen_Set_New();
    if (!stack) {
      Xen_DEL_REF(self_id);
      return NULL;
    }
  } else {
    if (Xen_Set_Has(stack, self_id)) {
      Xen_Instance* string = Xen_String_From_CString("<Vector(...)>");
      if (!string) {
        Xen_DEL_REF(self_id);
        Xen_DEL_REF(stack);
        return NULL;
      }
      Xen_DEL_REF(self_id);
      Xen_DEL_REF(stack);
      return string;
    }
  }
  if (!Xen_Set_Push(stack, self_id)) {
    Xen_DEL_REF(self_id);
    Xen_DEL_REF(stack);
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)self;
  char* buffer = Xen_CString_Dup("<Vector(");
  if (!buffer) {
    Xen_DEL_REF(self_id);
    Xen_DEL_REF(stack);
    return NULL;
  }
  Xen_size_t buflen = 9;
  for (Xen_size_t i = 0; i < Xen_SIZE(vector); i++) {
    Xen_Instance* value_inst = Xen_Vector_Peek_Index(self, i);
    Xen_Instance* value_string = Xen_Attr_Raw_Stack(value_inst, stack);
    if (!value_string) {
      Xen_DEL_REF(self_id);
      Xen_DEL_REF(stack);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = Xen_CString_Dup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_DEL_REF(self_id);
      Xen_DEL_REF(stack);
      Xen_DEL_REF(value_string);
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_DEL_REF(value_string);
    buflen += Xen_CString_Len(value);
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_DEL_REF(self_id);
      Xen_DEL_REF(stack);
      Xen_Dealloc((void*)value);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buffer = temp;
    strcat(buffer, value);
    Xen_Dealloc((void*)value);
    if (i != Xen_SIZE(vector) - 1) {
      buflen += 2;
      char* tem = Xen_Realloc(buffer, buflen);
      if (!tem) {
        Xen_DEL_REF(self_id);
        Xen_DEL_REF(stack);
        Xen_Dealloc(buffer);
        return NULL;
      }
      buffer = tem;
      strcat(buffer, ", ");
    }
  }
  Xen_DEL_REF(self_id);
  Xen_DEL_REF(stack);
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
}

static Xen_Instance* vector_opr_get_index(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != &Xen_Number_Implement)
    return NULL;
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  return Xen_ADD_REF(((Xen_Vector*)self)->values[index]);
}

static Xen_Instance* vector_iter(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Vector_Iterator_New(self);
}

static Xen_Instance* vector_opr_set_index(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 2)
    return NULL;
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != &Xen_Number_Implement)
    return NULL;
  Xen_Instance* value_inst = Xen_Vector_Peek_Index(args, 1);
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  Xen_DEL_REF(((Xen_Vector*)self)->values[index]);
  ((Xen_Vector*)self)->values[index] = Xen_ADD_REF(value_inst);
  return nil;
}

static Xen_Instance* vector_push(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* val = Xen_Attr_Index_Size_Get(args, 0);
  if (!Xen_Vector_Push(self, val)) {
    Xen_DEL_REF(val);
    return NULL;
  }
  Xen_DEL_REF(val);
  return nil;
}

struct __Implement Xen_Vector_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Vector",
    .__inst_size = sizeof(struct Xen_Vector_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = vector_alloc,
    .__create = vector_create,
    .__destroy = vector_destroy,
    .__string = vector_string,
    .__raw = vector_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

int Xen_Vector_Init() {
  if (!Xen_VM_Store_Global("vector", (Xen_Instance*)&Xen_Vector_Implement)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__get_index", vector_opr_get_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__set_index", vector_opr_set_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__iter", vector_iter, nil) ||
      !Xen_VM_Store_Native_Function(props, "push", vector_push, nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Vector_Implement.__props = props;
  return 1;
}

void Xen_Vector_Finish() {
  Xen_DEL_REF(Xen_Vector_Implement.__props);
}
