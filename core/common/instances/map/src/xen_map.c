#include <stdlib.h>

#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_map.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"

int Xen_Map_Push_Pair(Xen_Instance *map_inst, Xen_Map_Pair pair) {
  if (!pair.key || !pair.value || !Xen_TYPE(pair.key)->__hash) { return 0; }
  Xen_Map *map = (Xen_Map *)map_inst;
  if (!vm_call_native_function(pair.key->__impl->__hash, pair.key, nil)) { return 0; }
  Xen_Instance *hash_inst = xen_register_prop_get("__expose_hash", 0);
  if (!hash_inst) { return 0; }
  int hash_index = Xen_Number_As_ULong(hash_inst) % map->map_capacity;
  Xen_DEL_REF(hash_inst);
  struct __Map_Node *current = map->map_buckets[hash_index];
  while (current) {
    Xen_Instance *eval = Xen_Operator_Eval_Pair(current->value, pair.value, Xen_EQ);
    if_nil_eval(eval) { return 0; }
    if (eval == Xen_True) {
      Xen_DEL_REF(current->value);
      current->value = pair.value;
      Xen_ADD_REF(pair.value);
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
