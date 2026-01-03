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
#include "xen_boolean.h"
#include "xen_cstrings.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include "xen_vector_iterator.h"

#define XEN_MAP_CAPACITY 128

static void map_trace(Xen_GCHeader* h) {
  Xen_Map* map = (Xen_Map*)h;
  if (map->map_keys)
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)map->map_keys);
  if (map->map_buckets) {
    for (size_t i = 0; i < map->map_capacity; i++) {
      struct __Map_Node* current = map->map_buckets[i];
      while (current) {
        Xen_GC_Trace_GCHeader((Xen_GCHeader*)current->key);
        Xen_GC_Trace_GCHeader((Xen_GCHeader*)current->value);
        current = current->next;
      }
    }
  }
}

static Xen_Instance* map_alloc(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)Xen_Instance_Alloc(xen_globals->implements->map);
  if (!map) {
    return NULL;
  }
  Xen_IGC_Push((Xen_Instance*)map);
  map->map_buckets = Xen_Alloc(XEN_MAP_CAPACITY * sizeof(struct __Map_Node*));
  for (size_t i = 0; i < XEN_MAP_CAPACITY; i++) {
    map->map_buckets[i] = NULL;
  }
  map->map_keys = Xen_GCHandle_New_From((Xen_GCHeader*)__instance_new(
      xen_globals->implements->vector, nil, nil, 0));
  if (!map->map_keys) {
    Xen_Dealloc(map->map_buckets);
    map->map_buckets = NULL;
    Xen_IGC_Pop();
    return NULL;
  }
  map->map_capacity = XEN_MAP_CAPACITY;
  Xen_IGC_Pop();
  return (Xen_Instance*)map;
}

static Xen_Instance* map_destroy(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  if (map->map_buckets) {
    for (size_t i = 0; i < map->map_capacity; i++) {
      struct __Map_Node* current = map->map_buckets[i];
      while (current) {
        struct __Map_Node* temp = current;
        current = current->next;
        Xen_GCHandle_Free(current->key);
        Xen_GCHandle_Free(current->value);
        Xen_Dealloc(temp);
      }
    }
    Xen_Dealloc(map->map_buckets);
  }
  Xen_GCHandle_Free(map->map_keys);
  return nil;
}

static Xen_Instance* map_string(Xen_Instance* self, Xen_Instance* args,
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
      Xen_Instance* string = Xen_String_From_CString("<Map(...)>");
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
  Xen_Map* map = (Xen_Map*)self;
  char* buffer = Xen_CString_Dup("<Map(");
  if (!buffer) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_size_t buflen = 6;
  for (Xen_size_t i = 0; i < Xen_SIZE(map->map_keys); i++) {
    Xen_Instance* key_inst =
        Xen_Vector_Peek_Index((Xen_Instance*)map->map_keys->ptr, i);
    Xen_Instance* value_inst = Xen_Map_Get(self, key_inst);
    Xen_Instance* key_string = Xen_Attr_Raw_Stack(key_inst, stack);
    if (!key_string) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* value_string = Xen_Attr_Raw_Stack(value_inst, stack);
    if (!value_string) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* key = Xen_CString_Dup(Xen_String_As_CString(key_string));
    if (!key) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(buffer);
      return NULL;
    }
    const char* value = Xen_CString_Dup(Xen_String_As_CString(value_string));
    if (!value) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc((void*)key);
      Xen_Dealloc(buffer);
      return NULL;
    }
    buflen += Xen_CString_Len(key) + Xen_CString_Len(value) + 2;
    char* temp = Xen_Realloc(buffer, buflen);
    if (!temp) {
      Xen_IGC_XPOP(roots);
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

static Xen_Instance* map_opr_get_index(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  Xen_IGC_Push(key);
  Xen_Instance* value = Xen_Map_Get(self, key);
  if (!value) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  return value;
}

static Xen_Instance* map_opr_set_index(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  Xen_IGC_XPUSH(key, roots);
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 1);
  Xen_IGC_XPUSH(value, roots);
  if (!Xen_Map_Push_Pair(self, (Xen_Map_Pair){key, value})) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPOP(roots);
  return nil;
}

static Xen_Instance* map_opr_has(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Map_Has(self, value)) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* map_iter(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  return Xen_Vector_Iterator_New((Xen_Instance*)map->map_keys->ptr);
}

static Xen_Instance* map_push(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 2) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 1);
  if (!Xen_Map_Push_Pair(self, (Xen_Map_Pair){key, value})) {
    return NULL;
  }
  return nil;
}

static struct __Implement __Map_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Map",
    .__inst_size = sizeof(struct Xen_Map_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = map_trace,
    .__props = NULL,
    .__alloc = map_alloc,
    .__create = NULL,
    .__destroy = map_destroy,
    .__string = map_string,
    .__raw = map_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_Map_GetImplement(void) {
  return &__Map_Implement;
}

int Xen_Map_Init(void) {
  if (!Xen_VM_Store_Global("map",
                           (Xen_Instance*)xen_globals->implements->map)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__get_index", map_opr_get_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__set_index", map_opr_set_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__has", map_opr_has, nil) ||
      !Xen_VM_Store_Native_Function(props, "__iter", map_iter, nil) ||
      !Xen_VM_Store_Native_Function(props, "push", map_push, nil)) {
    return 0;
  }
  __Map_Implement.__props = Xen_GCHandle_New_From((Xen_GCHeader*)props);
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Map_Finish(void) {
  Xen_GCHandle_Free(__Map_Implement.__props);
}
