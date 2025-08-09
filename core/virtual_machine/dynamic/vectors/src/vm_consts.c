#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "vm_consts.h"

vm_Consts_ptr vm_consts_new() {
  vm_Consts_ptr consts = malloc(sizeof(vm_Consts));
  if (!consts) { return NULL; }
  consts->c_names = NULL;
  consts->c_names_size = 0;
  consts->c_names_capacity = 0;
  consts->c_instances = NULL;
  consts->c_instances_size = 0;
  consts->c_instances_capacity = 0;
  return consts;
}

bool vm_consts_push_name(vm_Consts_ptr consts, char *c_name) {
  if (!consts || !c_name) { return false; }
  if (consts->c_names_size >= consts->c_names_capacity) {
    size_t new_capacity =
        consts->c_names_capacity == 0 ? 4 : consts->c_names_capacity * 4;
    char **new_mem = realloc(consts->c_names, sizeof(char *) * new_capacity);
    if (!new_mem) { return false; }
    consts->c_names = new_mem;
    consts->c_names_capacity = new_capacity;
  }
  consts->c_names[consts->c_names_size++] = c_name;
  return true;
}
