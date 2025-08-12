#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
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

bool vm_consts_push_name(vm_Consts_ptr consts, const char *c_name) {
  if (!consts || !c_name) { return false; }
  if (consts->c_names_size >= consts->c_names_capacity) {
    size_t new_capacity =
        consts->c_names_capacity == 0 ? 4 : consts->c_names_capacity * 2;
    const char **new_mem = realloc(consts->c_names, sizeof(char *) * new_capacity);
    if (!new_mem) { return false; }
    consts->c_names = new_mem;
    consts->c_names_capacity = new_capacity;
  }
  consts->c_names[consts->c_names_size] = strdup(c_name);
  if (!consts->c_names[consts->c_names_size]) { return false; }
  consts->c_names_size++;
  return true;
}

bool vm_consts_push_instance(vm_Consts_ptr consts, struct __Instance *c_instance,
                             bool ref) {
  if (!consts || !c_instance) { return false; };
  if (consts->c_instances_size >= consts->c_instances_capacity) {
    size_t new_capacity =
        consts->c_instances_capacity == 0 ? 4 : consts->c_instances_capacity * 2;
    struct __Instance **new_mem =
        realloc(consts->c_instances, sizeof(struct __Instance *) * new_capacity);
    if (!new_mem) { return false; }
    consts->c_instances = new_mem;
    consts->c_instances_capacity = new_capacity;
  }
  if (ref) Xen_ADD_REF(c_instance);
  consts->c_instances[consts->c_instances_size++] = c_instance;
  return true;
}

void vm_consts_free(vm_Consts_ptr consts) {
  if (!consts) { return; }
  for (int i = 0; i < consts->c_names_size; i++) {
    free((void *)consts->c_names[i]);
  }
  for (int i = 0; i < consts->c_instances_size; i++) {
    Xen_DEL_REF(consts->c_instances[i]);
  }
  free(consts->c_names);
  free(consts->c_instances);
  free(consts);
  consts = NULL;
}
