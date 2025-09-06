#include "implement.h"
#include "instance.h"
#include "vm.h"
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
  struct __Map_Node *current = map->map_buckets[hash_index];
  while (current) {
    if (strcmp(current.)) }
  return 1;
}
