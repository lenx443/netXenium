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
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_string.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"

static Xen_Instance* vector_create(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector* vector = (Xen_Vector*)self;
  vector->values = NULL;
  vector->capacity = 0;
  return nil;
}

static Xen_Instance* vector_destroy(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector* vector = (Xen_Vector*)self;
  for (size_t i = 0; i < vector->__size; i++) {
    Xen_DEL_REF(vector->values[i]);
  }
  free(vector->values);
  return nil;
}

static Xen_Instance* vector_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector* vector = (Xen_Vector*)self;
  char* buffer = strdup("<Vector(");
  if (!buffer) {
    return NULL;
  }
  Xen_size_t buflen = 9;
  for (Xen_size_t i = 0; i < Xen_SIZE(vector); i++) {
    Xen_Instance* value_inst = Xen_Vector_Peek_Index(self, i);
    Xen_Instance* value_string = Xen_Attr_Raw(value_inst);
    if (!value_string) {
      free(buffer);
      return NULL;
    }
    const char* value = strdup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_DEL_REF(value_string);
      free(buffer);
      return NULL;
    }
    Xen_DEL_REF(value_string);
    buflen += strlen(value);
    char* temp = realloc(buffer, buflen);
    if (!temp) {
      free((void*)value);
      free(buffer);
      return NULL;
    }
    buffer = temp;
    strcat(buffer, value);
    free((void*)value);
    if (i != Xen_SIZE(vector) - 1) {
      buflen += 2;
      char* tem = realloc(buffer, buflen);
      if (!tem) {
        free(buffer);
        return NULL;
      }
      buffer = tem;
      strcat(buffer, ", ");
    }
  }
  buflen += 2;
  char* temp = realloc(buffer, buflen);
  if (!temp) {
    free(buffer);
    return NULL;
  }
  buffer = temp;
  strcat(buffer, ")>");
  buffer[buflen - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  if (!string) {
    free(buffer);
    return NULL;
  }
  free(buffer);
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

struct __Implement Xen_Vector_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Vector",
    .__inst_size = sizeof(struct Xen_Vector_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
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
  Xen_Instance* props = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!props) {
    return 0;
  }
  if (!vm_define_native_function(props, "__get_index", vector_opr_get_index,
                                 nil) ||
      !vm_define_native_function(props, "__set_index", vector_opr_set_index,
                                 nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Vector_Implement.__props = props;
  return 1;
}

void Xen_Vector_Finish() {
  Xen_DEL_REF(Xen_Vector_Implement.__props);
}
