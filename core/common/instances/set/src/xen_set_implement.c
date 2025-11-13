#include "xen_set_implement.h"
#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_set.h"
#include "xen_set_instance.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include <string.h>

#define XEN_SET_CAPACITY 128

static Xen_Instance* set_alloc(ctx_id_t id, Xen_Instance* self,
                               Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Set* set = (Xen_Set*)Xen_Instance_Alloc(&Xen_Set_Implement);
  if (!set) {
    return NULL;
  }
  set->buckets = Xen_Alloc(XEN_SET_CAPACITY * sizeof(struct __Set_Node*));
  if (!set->buckets) {
    Xen_DEL_REF(set);
    return NULL;
  }
  for (Xen_size_t i = 0; i < XEN_SET_CAPACITY; i++) {
    set->buckets[i] = NULL;
  }
  set->values = Xen_Vector_New();
  if (!set->values) {
    Xen_Dealloc(set->buckets);
    set->buckets = NULL;
    Xen_DEL_REF(set);
    return NULL;
  }
  set->capacity = XEN_SET_CAPACITY;
  return (Xen_Instance*)set;
}

static Xen_Instance* set_destroy(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Set* set = (Xen_Set*)self;
  if (set->values) {
    Xen_DEL_REF(set->values);
  }
  if (set->buckets) {
    for (size_t i = 0; i < set->capacity; i++) {
      struct __Set_Node* current = set->buckets[i];
      while (current) {
        struct __Set_Node* temp = current;
        current = current->next;
        Xen_DEL_REF(temp->value);
        Xen_Dealloc(temp);
      }
    }
    Xen_Dealloc(set->buckets);
  }
  return nil;
}

static Xen_Instance* set_string(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Set* set = (Xen_Set*)self;
  char* buffer = strdup("<Set(");
  if (!buffer) {
    return NULL;
  }
  Xen_size_t buflen = 6;
  for (Xen_size_t i = 0; i < Xen_SIZE(set->values); i++) {
    Xen_Instance* value_inst = Xen_Vector_Peek_Index(set->values, i);
    Xen_Instance* value_string = Xen_Attr_Raw(value_inst);
    if (!value_string) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = strdup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_DEL_REF(value_string);
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_DEL_REF(value_string);
    buflen += strlen(value) + 2;
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_Dealloc((void*)value);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buffer = temp;
    strcat(buffer, value);
    Xen_Dealloc((void*)value);
    if (i != Xen_SIZE(set->values) - 1) {
      buflen += 2;
      char* tem = Xen_Realloc(buffer, buflen);
      if (!tem) {
        Xen_Dealloc(buffer);
        return NULL;
      }
      buffer = tem;
      strcat(buffer, ", ");
    }
  }
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

static Xen_Instance* set_push(ctx_id_t id, Xen_Instance* self,
                              Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value = Xen_Attr_Index_Size_Get(args, 0);
  if (!Xen_Set_Push(self, value)) {
    Xen_DEL_REF(value);
    return NULL;
  }
  Xen_DEL_REF(value);
  return nil;
}

static Xen_Instance* set_opr_has(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Set_Has(self, value)) {
    Xen_DEL_REF(value);
    return Xen_True;
  }
  Xen_DEL_REF(value);
  return Xen_False;
}

Xen_Implement Xen_Set_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Set",
    .__inst_size = sizeof(struct Xen_Set_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = set_alloc,
    .__create = NULL,
    .__destroy = set_destroy,
    .__string = set_string,
    .__raw = set_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

int Xen_Set_Init() {
  if (!Xen_VM_Store_Global("set", (Xen_Instance*)&Xen_Set_Implement)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__has", set_opr_has, nil) ||
      !Xen_VM_Store_Native_Function(props, "push", set_push, nil)) {
    return 0;
  }
  Xen_Set_Implement.__props = props;
  return 1;
}

void Xen_Set_Finish() {
  Xen_DEL_REF(Xen_Set_Implement.__props);
}
