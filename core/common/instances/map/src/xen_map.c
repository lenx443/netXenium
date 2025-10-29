#include <stddef.h>
#include <stdlib.h>

#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"

Xen_Instance* Xen_Map_New(size_t capacity) {
  Xen_Map* map = (Xen_Map*)__instance_new(&Xen_Map_Implement, nil, 0);
  if (!map) {
    return NULL;
  }
  map->map_buckets = malloc(capacity * sizeof(struct __Map_Node*));
  if (!map->map_buckets) {
    return NULL;
  }
  for (size_t i = 0; i < capacity; i++) {
    map->map_buckets[i] = NULL;
  }
  map->map_keys = __instance_new(&Xen_Vector_Implement, nil, 0);
  if (!map->map_keys) {
    free(map->map_buckets);
    return NULL;
  }
  map->map_capacity = capacity;
  return (Xen_Instance*)map;
}

Xen_Instance* Xen_Map_From_Pairs_With_Size(size_t size, Xen_Map_Pair* pairs,
                                           size_t capacity) {
  Xen_Instance* map = Xen_Map_New(capacity);
  for (size_t i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair(map, pairs[i])) {
      Xen_DEL_REF(map);
      return NULL;
    }
  }
  return (Xen_Instance*)map;
}

Xen_Instance* Xen_Map_From_Pairs_Str_With_Size(size_t size,
                                               Xen_Map_Pair_Str* pairs,
                                               size_t capacity) {
  Xen_Instance* map = Xen_Map_New(capacity);
  for (size_t i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair_Str((Xen_Instance*)map, pairs[i])) {
      Xen_DEL_REF(map);
      return NULL;
    }
  }
  return (Xen_Instance*)map;
}

int Xen_Map_Push_Pair(Xen_Instance* map_inst, Xen_Map_Pair pair) {
  if (!pair.key || !pair.value || !Xen_TYPE(pair.key)->__hash) {
    return 0;
  }
  Xen_Map* map = (Xen_Map*)map_inst;

  Xen_Instance* hash_inst =
      vm_call_native_function(pair.key->__impl->__hash, pair.key, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return 0;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  Xen_DEL_REF(hash_inst);
  struct __Map_Node* current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_TYPE(pair.key) == &Xen_String_Implement) {
      if (Xen_TYPE(current->key) == &Xen_String_Implement &&
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
      Xen_DEL_REF(current->value);
      current->value = Xen_ADD_REF(pair.value);
      return 1;
    }
    current = current->next;
  }
  struct __Map_Node* new_node = malloc(sizeof(struct __Map_Node));
  if (!new_node) {
    return 0;
  }
  if (!Xen_Vector_Push(map->map_keys, pair.key)) {
    free(new_node);
    return 0;
  }
  new_node->key = pair.key;
  new_node->value = pair.value;
  Xen_ADD_REF(pair.key);
  Xen_ADD_REF(pair.value);

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
    Xen_DEL_REF(key_inst);
    return 0;
  }
  Xen_DEL_REF(key_inst);
  return 1;
}

int Xen_Map_Push_Map(Xen_Instance* map_dst, Xen_Instance* map_src) {
  if (!map_dst || !map_src) {
    return 0;
  }
  Xen_Instance* src_keys = Xen_Map_Keys(map_src);
  for (size_t i = 0; i < Xen_SIZE(src_keys); i++) {
    Xen_Instance* key = Xen_Operator_Eval_Pair_Steal2(
        src_keys, Xen_Number_From_ULong(i), Xen_OPR_GET_INDEX);
    Xen_Instance* value = Xen_Map_Get(map_src, key);
    if (!Xen_Map_Push_Pair(map_dst, (Xen_Map_Pair){key, value})) {
      Xen_DEL_REF(value);
      Xen_DEL_REF(key);
      return 0;
    }
    Xen_DEL_REF(value);
    Xen_DEL_REF(key);
  }
  Xen_DEL_REF(src_keys);
  return 1;
}

Xen_Instance* Xen_Map_Get(Xen_Instance* map_inst, Xen_Instance* key) {
  Xen_Map* map = (Xen_Map*)map_inst;
  Xen_Instance* hash_inst =
      vm_call_native_function(key->__impl->__hash, key, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return NULL;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  Xen_DEL_REF(hash_inst);
  struct __Map_Node* current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_TYPE(key) == &Xen_String_Implement) {
      if (Xen_TYPE(current->key) == &Xen_String_Implement &&
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
      return Xen_ADD_REF(current->value);
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
  Xen_DEL_REF(key_inst);
  return result;
}

Xen_Instance* Xen_Map_Keys(Xen_Instance* map) {
  if (!map || Xen_Nil_Eval(map)) {
    return NULL;
  }
  return Xen_ADD_REF(((Xen_Map*)map)->map_keys);
}
