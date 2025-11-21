#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "attrs.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_vector.h"

Xen_Instance* Xen_Map_New() {
  Xen_Map* map = (Xen_Map*)__instance_new(&Xen_Map_Implement, nil, nil, 0);
  if (!map) {
    return NULL;
  }
  return (Xen_Instance*)map;
}

Xen_Instance* Xen_Map_From_Pairs_With_Size(size_t size, Xen_Map_Pair* pairs) {
  Xen_Instance* map = Xen_Map_New();
  for (size_t i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair(map, pairs[i])) {
      return NULL;
    }
  }
  return (Xen_Instance*)map;
}

Xen_Instance* Xen_Map_From_Pairs_Str_With_Size(size_t size,
                                               Xen_Map_Pair_Str* pairs) {
  Xen_Instance* map = Xen_Map_New();
  for (size_t i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair_Str((Xen_Instance*)map, pairs[i])) {
      return NULL;
    }
  }
  return (Xen_Instance*)map;
}

int Xen_Map_Push_Pair(Xen_Instance* map_inst, Xen_Map_Pair pair) {
  if (!pair.key || !pair.value || !Xen_IMPL(pair.key)->__hash) {
    return 0;
  }
  Xen_Map* map = (Xen_Map*)map_inst;

  Xen_Instance* hash_inst =
      Xen_VM_Call_Native_Function(pair.key->__impl->__hash, pair.key, nil, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return 0;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  struct __Map_Node* current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_IMPL(pair.key) == &Xen_String_Implement) {
      if (Xen_IMPL(current->key) == &Xen_String_Implement &&
          strcmp(((Xen_String*)current->key)->characters,
                 ((Xen_String*)pair.key)->characters) == 0) {
        eval = Xen_True;
      } else {
        eval = Xen_False;
      }
    } else {
      eval = Xen_Operator_Eval_Pair(current->key, pair.key, Xen_OPR_EQ);
      if (!eval) {
        return 0;
      }
    }
    if (eval == Xen_True) {
      Xen_GC_Write_Field((Xen_GCHeader*)current,
                         (Xen_GCHeader**)&current->value,
                         (Xen_GCHeader*)pair.value);
      return 1;
    }
    current = current->next;
  }
  struct __Map_Node* new_node = Xen_Alloc(sizeof(struct __Map_Node));
  if (!new_node) {
    return 0;
  }
  if (!Xen_Vector_Push(map->map_keys, pair.key)) {
    Xen_Dealloc(new_node);
    return 0;
  }
  Xen_GC_Write_Field((Xen_GCHeader*)map, (Xen_GCHeader**)&new_node->key,
                     (Xen_GCHeader*)pair.key);
  Xen_GC_Write_Field((Xen_GCHeader*)map, (Xen_GCHeader**)&new_node->value,
                     (Xen_GCHeader*)pair.value);

  new_node->next = map->map_buckets[hash_index];
  map->map_buckets[hash_index] = new_node;
  return 1;
}

int Xen_Map_Push_Pair_Str(Xen_Instance* map, Xen_Map_Pair_Str pair) {
  if (!pair.key || !pair.value) {
    return 0;
  }
  Xen_Instance* key_inst = Xen_String_From_CString(pair.key);
  if (!key_inst) {
    return 0;
  }
  if (!Xen_Map_Push_Pair(map, (Xen_Map_Pair){key_inst, pair.value})) {
    return 0;
  }
  return 1;
}

int Xen_Map_Push_Map(Xen_Instance* map_dst, Xen_Instance* map_src) {
  if (!map_dst || !map_src) {
    return 0;
  }
  Xen_Instance* src_keys = Xen_Map_Keys(map_src);
  for (size_t i = 0; i < Xen_SIZE(src_keys); i++) {
    Xen_Instance* key = Xen_Attr_Index_Size_Get(src_keys, i);
    Xen_Instance* value = Xen_Map_Get(map_src, key);
    if (!Xen_Map_Push_Pair(map_dst, (Xen_Map_Pair){key, value})) {
      return 0;
    }
  }
  return 1;
}

Xen_Instance* Xen_Map_Get(Xen_Instance* map_inst, Xen_Instance* key) {
  if (!key || !Xen_IMPL(key)->__hash) {
    return NULL;
  }
  Xen_Map* map = (Xen_Map*)map_inst;
  Xen_Instance* hash_inst =
      Xen_VM_Call_Native_Function(key->__impl->__hash, key, nil, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return NULL;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  struct __Map_Node* current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_IMPL(key) == &Xen_String_Implement) {
      if (Xen_IMPL(current->key) == &Xen_String_Implement &&
          strcmp(((Xen_String*)current->key)->characters,
                 ((Xen_String*)key)->characters) == 0) {
        eval = Xen_True;
      } else {
        eval = Xen_False;
      }
    } else {
      eval = Xen_Operator_Eval_Pair(current->key, key, Xen_OPR_EQ);
      if (!eval) {
        return NULL;
      }
    }
    if (eval == Xen_True) {
      return current->value;
    }
    current = current->next;
  }
  return NULL;
}

Xen_Instance* Xen_Map_Get_Str(Xen_Instance* map, const char* key) {
  if (!key) {
    return NULL;
  }
  Xen_Instance* key_inst = Xen_String_From_CString(key);
  if (!key_inst) {
    return NULL;
  }
  Xen_Instance* result = Xen_Map_Get(map, key_inst);
  return result;
}

Xen_Instance* Xen_Map_Keys(Xen_Instance* map) {
  if (!map || Xen_Nil_Eval(map)) {
    return NULL;
  }
  return ((Xen_Map*)map)->map_keys;
}

int Xen_Map_Has(Xen_Instance* map_inst, Xen_Instance* key) {
  if (!key || !Xen_IMPL(key)->__hash) {
    return 0;
  }
  Xen_Map* map = (Xen_Map*)map_inst;
  Xen_Instance* hash_inst =
      Xen_VM_Call_Native_Function(key->__impl->__hash, key, nil, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return 0;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  struct __Map_Node* current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_IMPL(key) == &Xen_String_Implement) {
      if (Xen_IMPL(current->key) == &Xen_String_Implement &&
          strcmp(((Xen_String*)current->key)->characters,
                 ((Xen_String*)key)->characters) == 0) {
        eval = Xen_True;
      } else {
        eval = Xen_False;
      }
    } else {
      eval = Xen_Operator_Eval_Pair(current->key, key, Xen_OPR_EQ);
      if (!eval) {
        return 0;
      }
    }
    if (eval == Xen_True) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}
