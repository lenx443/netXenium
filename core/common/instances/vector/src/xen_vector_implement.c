#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"
#include "xen_vector_iterator.h"

static void vector_trace(Xen_GCHeader* h) {
  Xen_Vector* vector = (Xen_Vector*)h;
  int __debug = 0;
  for (size_t i = 0; i < vector->__size; i++) {
    if (__debug) {
      printf("iter = %lu\n", i);
    }
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)vector->values[i]->ptr);
  }
}

static Xen_Instance* vector_alloc(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  }
  Xen_Vector* vector =
      (Xen_Vector*)Xen_Instance_Alloc(xen_globals->implements->vector);
  if (!vector) {
    return NULL;
  }
  vector->values = NULL;
  vector->capacity = 0;
  return (Xen_Instance*)vector;
}

static Xen_Instance* vector_create(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* iterable = Xen_Tuple_Get_Index(args, 0);
    if (self == iterable) {
      return nil;
    }
    Xen_IGC_Push(iterable);
    Xen_Instance* iter = Xen_Attr_Iter(iterable);
    if (!iter) {
      return NULL;
    }
    Xen_IGC_Push(iter);
    Xen_Instance* value = NULL;
    while ((value = Xen_Attr_Next(iter)) != NULL) {
      if (!Xen_Vector_Push(self, value)) {
        Xen_IGC_XPOP(2);
        return NULL;
      }
    }
    Xen_IGC_XPOP(2);
    if (!Xen_VM_Except_Active() ||
        strcmp(((Xen_Except*)(*xen_globals->vm)->except.except->ptr)->type,
               "RangeEnd") != 0) {
      return NULL;
    }
    (*xen_globals->vm)->except.active = 0;
  }
  return nil;
}

static Xen_Instance* vector_destroy(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector* vector = (Xen_Vector*)self;
  for (Xen_size_t i = 0; i < vector->__size; i++) {
    Xen_GCHandle_Free(vector->values[i]);
  }
  Xen_Dealloc(vector->values);
  return nil;
}

static Xen_Instance* vector_string(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_size_t roots = 0;
  Xen_Instance* self_id = Xen_Number_From_Pointer(self);
  if (!self_id) {
    return NULL;
  }
  Xen_IGC_XPUSH(self_id, roots);
  Xen_Instance* stack = NULL;
  if (Xen_SIZE(args) > 1) {
    Xen_IGC_XPOP(roots);
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    stack = Xen_Tuple_Get_Index(args, 0);
    if (Xen_IMPL(stack) != xen_globals->implements->map) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
  }
  if (!stack) {
    stack = Xen_Map_New();
    if (!stack) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    Xen_IGC_XPUSH(stack, roots);
  } else {
    if (Xen_Map_Has(stack, self_id)) {
      Xen_Instance* string = Xen_String_From_CString("<Vector(...)>");
      if (!string) {
        Xen_IGC_XPOP(roots);
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return string;
    }
  }
  if (!Xen_Map_Push_Pair(stack, (Xen_Map_Pair){self_id, nil})) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)self;
  char* buffer = Xen_CString_Dup("<Vector(");
  if (!buffer) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_size_t buflen = 9;
  for (Xen_size_t i = 0; i < Xen_SIZE(vector); i++) {
    Xen_Instance* value_inst = Xen_Vector_Peek_Index(self, i);
    Xen_Instance* value_string = Xen_Attr_Raw_Stack(value_inst, stack);
    if (!value_string) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = Xen_CString_Dup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buflen += Xen_CString_Len(value);
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_IGC_XPOP(roots);
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
        Xen_IGC_XPOP(roots);
        Xen_Dealloc(buffer);
        return NULL;
      }
      buffer = tem;
      strcat(buffer, ", ");
    }
  }
  Xen_IGC_XPOP(roots);
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

static Xen_Instance* vector_opr_get_index(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* index_inst = Xen_Tuple_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != xen_globals->implements->number)
    return NULL;
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  return (Xen_Instance*)((Xen_Vector*)self)->values[index]->ptr;
}

static Xen_Instance* vector_iter(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Vector_Iterator_New(self);
}

static Xen_Instance* vector_opr_set_index(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 2)
    return NULL;
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != xen_globals->implements->number)
    return NULL;
  Xen_Instance* value_inst = Xen_Vector_Peek_Index(args, 1);
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  ((Xen_Vector*)self)->values[index]->ptr = (Xen_GCHeader*)value_inst;
  return nil;
}

static Xen_Instance* vector_push(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* val = Xen_Tuple_Get_Index(args, 0);
  if (!Xen_Vector_Push(self, val)) {
    return NULL;
  }
  return nil;
}

static Xen_Instance* vector_pop(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Vector_Pop(self);
}

static Xen_Instance* vector_top(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Vector_Top(self);
}

static struct __Implement __Vector_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Vector",
    .__inst_size = sizeof(struct Xen_Vector_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = vector_trace,
    .__props = NULL,
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

struct __Implement* Xen_Vector_GetImplement(void) {
  return &__Vector_Implement;
}

int Xen_Vector_Init(void) {
  if (!Xen_VM_Store_Global("vector",
                           (Xen_Instance*)xen_globals->implements->vector)) {
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
      !Xen_VM_Store_Native_Function(props, "push", vector_push, nil) ||
      !Xen_VM_Store_Native_Function(props, "pop", vector_pop, nil) ||
      !Xen_VM_Store_Native_Function(props, "top", vector_top, nil)) {
    return 0;
  }
  __Vector_Implement.__props = Xen_GCHandle_New_From((Xen_GCHeader*)props);
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Vector_Finish(void) {
  Xen_GCHandle_Free(__Vector_Implement.__props);
}
