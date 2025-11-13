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
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"

#define XEN_MAP_CAPACITY 128

static Xen_Instance* map_alloc(ctx_id_t id, Xen_Instance* self,
                               Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)Xen_Instance_Alloc(&Xen_Map_Implement);
  if (!map) {
    return NULL;
  }
  map->map_buckets = Xen_Alloc(XEN_MAP_CAPACITY * sizeof(struct __Map_Node*));
  if (!map->map_buckets) {
    Xen_DEL_REF(map);
    return NULL;
  }
  for (size_t i = 0; i < XEN_MAP_CAPACITY; i++) {
    map->map_buckets[i] = NULL;
  }
  map->map_keys = __instance_new(&Xen_Vector_Implement, nil, nil, 0);
  if (!map->map_keys) {
    Xen_Dealloc(map->map_buckets);
    map->map_buckets = NULL;
    Xen_DEL_REF(map);
    return NULL;
  }
  map->map_capacity = XEN_MAP_CAPACITY;
  return (Xen_Instance*)map;
}

static Xen_Instance* map_destroy(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  if (map->map_keys) {
    Xen_DEL_REF(map->map_keys);
  }
  if (map->map_buckets) {
    for (size_t i = 0; i < map->map_capacity; i++) {
      struct __Map_Node* current = map->map_buckets[i];
      while (current) {
        struct __Map_Node* temp = current;
        current = current->next;
        Xen_DEL_REF(temp->key);
        Xen_DEL_REF(temp->value);
        Xen_Dealloc(temp);
      }
    }
    Xen_Dealloc(map->map_buckets);
  }
  return nil;
}

static Xen_Instance* map_string(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  char* buffer = strdup("<Map(");
  if (!buffer) {
    return NULL;
  }
  Xen_size_t buflen = 6;
  for (Xen_size_t i = 0; i < Xen_SIZE(map->map_keys); i++) {
    Xen_Instance* key_inst = Xen_Vector_Peek_Index(map->map_keys, i);
    Xen_Instance* value_inst = Xen_Map_Get(self, key_inst);
    Xen_Instance* key_string = Xen_Attr_Raw(key_inst);
    if (!key_string) {
      Xen_DEL_REF(value_inst);
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* value_string = Xen_Attr_Raw(value_inst);
    if (!value_string) {
      Xen_DEL_REF(key_string);
      Xen_DEL_REF(value_inst);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* key = strdup(Xen_String_As_CString(key_string));
    if (!key) {
      Xen_DEL_REF(value_string);
      Xen_DEL_REF(key_string);
      Xen_DEL_REF(value_inst);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = strdup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_DEL_REF(value_string);
      Xen_DEL_REF(key_string);
      Xen_DEL_REF(value_inst);
      Xen_Dealloc((void*)key);
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_DEL_REF(value_string);
    Xen_DEL_REF(key_string);
    Xen_DEL_REF(value_inst);
    buflen += strlen(key) + strlen(value) + 2;
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_Dealloc((void*)key);
      Xen_Dealloc((void*)value);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buffer = temp;
    strcat(buffer, key);
    strcat(buffer, ": ");
    strcat(buffer, value);
    Xen_Dealloc((void*)key);
    Xen_Dealloc((void*)value);
    if (i != Xen_SIZE(map->map_keys) - 1) {
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

static Xen_Instance* map_opr_get_index(ctx_id_t id, Xen_Instance* self,
                                       Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* value = Xen_Map_Get(self, key);
  if (!value) {
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_DEL_REF(key);
  return value;
}

static Xen_Instance* map_push(ctx_id_t id, Xen_Instance* self,
                              Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* key = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* value = Xen_Attr_Index_Size_Get(args, 1);
  if (!Xen_Map_Push_Pair(self, (Xen_Map_Pair){key, value})) {
    Xen_DEL_REF(value);
    Xen_DEL_REF(key);
    return NULL;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(key);
  return nil;
}

struct __Implement Xen_Map_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Map",
    .__inst_size = sizeof(struct Xen_Map_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = map_alloc,
    .__create = NULL,
    .__destroy = map_destroy,
    .__string = map_string,
    .__raw = map_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Map_Init() {
  if (!Xen_VM_Store_Global("map", (Xen_Instance*)&Xen_Map_Implement)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__get_index", map_opr_get_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "push", map_push, nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Map_Implement.__props = props;
  return 1;
}

void Xen_Map_Finish() {
  Xen_DEL_REF(Xen_Map_Implement.__props);
}
