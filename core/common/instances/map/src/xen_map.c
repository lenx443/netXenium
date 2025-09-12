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
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"

Xen_Instance *Xen_Map_New(size_t capacity) {
  Xen_Map *map = (Xen_Map *)__instance_new(&Xen_Map_Implement, nil, 0);
  if_nil_eval(map) { return nil; }
  map->map_buckets = malloc(capacity * sizeof(struct __Map_Node *));
  if (!map->map_buckets) { return nil; }
  for (int i = 0; i < capacity; i++) {
    map->map_buckets[i] = NULL;
  }
  map->map_keys = __instance_new(&Xen_Vector_Implement, nil, 0);
  if_nil_eval(map->map_keys) {
    free(map->map_buckets);
    return nil;
  }
  map->map_capacity = capacity;
  return (Xen_Instance *)map;
}

Xen_Instance *Xen_Map_From_Pairs_With_Size(size_t size, Xen_Map_Pair *pairs,
                                           size_t capacity) {
  Xen_Instance *map = Xen_Map_New(capacity);
  for (int i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair(map, pairs[i])) {
      Xen_DEL_REF(map);
      return nil;
    }
  }
  return (Xen_Instance *)map;
}

Xen_Instance *Xen_Map_From_Pairs_Str_With_Size(size_t size, Xen_Map_Pair_Str *pairs,
                                               size_t capacity) {
  Xen_Instance *map = Xen_Map_New(capacity);
  for (int i = 0; i < size; i++) {
    if (!Xen_Map_Push_Pair_Str((Xen_Instance *)map, pairs[i])) {
      Xen_DEL_REF(map);
      return nil;
    }
  }
  return (Xen_Instance *)map;
}

int Xen_Map_Push_Pair(Xen_Instance *map_inst, Xen_Map_Pair pair) {
  if (!pair.key || !pair.value || !Xen_TYPE(pair.key)->__hash) { return 0; }
  Xen_Map *map = (Xen_Map *)map_inst;

  Xen_Vector_Push(map->map_keys, pair.key);

  if (!vm_call_native_function(pair.key->__impl->__hash, pair.key, nil)) { return 0; }
  Xen_Instance *hash_inst = xen_register_prop_get("__expose_hash", 0);
  if_nil_eval(hash_inst) { return 0; }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  Xen_DEL_REF(hash_inst);
  struct __Map_Node *current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance *eval = Xen_Operator_Eval_Pair(current->key, pair.key, Xen_EQ);
    if_nil_eval(eval) { return 0; }
    if (eval == Xen_True) {
      Xen_DEL_REF(current->value);
      current->value = Xen_ADD_REF(pair.value);
      return 1;
    }
    current = current->next;
  }
  struct __Map_Node *new_node = malloc(sizeof(struct __Map_Node));
  if (!new_node) { return 0; }
  new_node->key = pair.key;
  new_node->value = pair.value;
  Xen_ADD_REF(pair.key);
  Xen_ADD_REF(pair.value);

  new_node->next = map->map_buckets[hash_index];
  map->map_buckets[hash_index] = new_node;
  return 1;
}

int Xen_Map_Push_Pair_Str(Xen_Instance *map, Xen_Map_Pair_Str pair) {
  if (!pair.key || !pair.value) { return 0; }
  Xen_Instance *key_inst = Xen_String_From_CString(pair.key);
  if_nil_eval(key_inst) { return 0; }
  if (!Xen_Map_Push_Pair(map, (Xen_Map_Pair){key_inst, pair.value})) {
    Xen_DEL_REF(key_inst);
    return 0;
  }
  Xen_DEL_REF(key_inst);
  return 1;
}

Xen_Instance *Xen_Map_Get(Xen_Instance *map_inst, Xen_Instance *key) {
  Xen_Map *map = (Xen_Map *)map_inst;
  if (!vm_call_native_function(Xen_TYPE(key)->__hash, key, nil)) { return nil; }
  Xen_Instance *hash_inst = xen_register_prop_get("__expose_hash", 0);
  if_nil_eval(hash_inst) { return nil; }
  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  Xen_DEL_REF(hash_inst);
  struct __Map_Node *current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance *eval = Xen_Operator_Eval_Pair(current->key, key, Xen_EQ);
    if_nil_eval(eval) { return nil; }
    if (eval == Xen_True) { return Xen_ADD_REF(current->value); }
    current = current->next;
  }
  return nil;
}

Xen_Instance *Xen_Map_Get_Str(Xen_Instance *map, char *key) {
  if (!key) { return nil; }
  Xen_Instance *key_inst = Xen_String_From_CString(key);
  if_nil_eval(key_inst) { return nil; }
  Xen_Instance *result = Xen_Map_Get(map, key_inst);
  Xen_DEL_REF(key_inst);
  return result;
}

Xen_Instance *Xen_Map_Keys(Xen_Instance *map) {
  if (!map || Xen_Nil_Eval(map)) { return nil; }
  return Xen_ADD_REF(((Xen_Map *)map)->map_keys);
}
