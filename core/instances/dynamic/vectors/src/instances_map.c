#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "instances_map.h"

static unsigned long hash(const char *str) {
  unsigned long hash = 0x1505;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

struct __Instances_Map *__instances_map_new(size_t capacity) {
  struct __Instances_Map *inst_map = malloc(sizeof(struct __Instances_Map));
  if (!inst_map) { return NULL; }
  inst_map->__buckets = malloc(sizeof(struct __Instances_Hash_Node *) * capacity);
  memset(inst_map, NULL, sizeof(struct __Instances_Hash_Node *) * capacity);
  if (!inst_map->__buckets) {
    free(inst_map);
    return NULL;
  }
  inst_map->__buckets_capacity = capacity;
  inst_map->__vec_inst = NULL;
  inst_map->__vec_size = 0;
  inst_map->__vec_capacity = 0;
  return inst_map;
}

bool __instances_map_add(struct __Instances_Map *inst_map, const char *key,
                         struct __Instance *inst) {
  if (!inst_map || !key || !inst) { return false; }
  int hash_index = hash(key) % inst_map->__buckets_capacity;
  struct __Instances_Hash_Node *current_node = inst_map->__buckets[hash_index];
  while (current_node) {
    if (strcmp(current_node->name, key) == 0) {
      free(current_node->name);
      free(current_node);
      break;
    }
    current_node = current_node->next;
  }
  struct __Instances_Hash_Node *hash_node = malloc(sizeof(struct __Instances_Hash_Node));
  if (!hash_node) { return false; }
  hash_node->name = strdup(key);
  if (!hash_node->name) {
    free(hash_node);
    return false;
  }
  if (inst_map->__vec_size < inst_map->__vec_capacity) {
    size_t capacity_new =
        inst_map->__vec_capacity == 0 ? 4 : inst_map->__vec_capacity * 2;
    struct __Instance **new_mem =
        realloc(inst_map->__vec_inst, sizeof(struct __Instance *) * capacity_new);
    if (!new_mem) {
      free(hash_node->name);
      free(hash_node);
      return false;
    }
    inst_map->__vec_inst = new_mem;
    inst_map->__vec_capacity = capacity_new;
  }
  hash_node->index = inst_map->__vec_size;
  hash_node->next = inst_map->__buckets[hash_index];
  inst_map->__buckets[hash_index] = hash_node;
  inst_map->__vec_inst[inst_map->__vec_size] = inst;
  inst_map->__vec_size++;
  Xen_ADD_REF(inst);
  return true;
}

Xen_INSTANCE *__instances_map_get(struct __Instances_Map *inst_map, const char *key) {
  if (!inst_map || !key) { return NULL; }
  int hash_index = hash(key) % inst_map->__buckets_capacity;
  struct __Instances_Hash_Node *current_node = inst_map->__buckets[hash_index];
  while (current_node) {
    if (strcmp(current_node->name, key) == 0)
      return inst_map->__vec_inst[current_node->index];
    current_node = current_node->next;
  }
  return NULL;
}

void __instances_map_free(struct __Instances_Map *inst_map) {
  if (!inst_map) { return; }
  for (int i = 0; i < inst_map->__buckets_capacity; i++) {
    struct __Instances_Hash_Node *current = inst_map->__buckets[i];
    while (current) {
      struct __Instances_Hash_Node *temp = current;
      current = current->next;
      free(temp->name);
      free(temp);
    }
  }
  free(inst_map->__buckets);
  for (int i = 0; i < inst_map->__vec_size; i++) {
    Xen_DEL_REF(inst_map->__vec_inst[i]);
  }
  free(inst_map->__vec_inst);
  free(inst_map);
}
