#ifndef __VM_CONSTS_H__
#define __VM_CONSTS_H__

#include <stdbool.h>
#include <stddef.h>

struct __Instance;

typedef struct {
  const char **c_names;
  size_t c_names_size;
  size_t c_names_capacity;

  struct __Instance **c_instances;
  size_t c_instances_size;
  size_t c_instances_capacity;
} vm_Consts;

typedef vm_Consts *vm_Consts_ptr;

vm_Consts_ptr vm_consts_new();
bool vm_consts_push_name(vm_Consts_ptr, const char *);
bool vm_consts_push_instance(vm_Consts_ptr, struct __Instance *);
void vm_consts_instances_free(vm_Consts_ptr);
void vm_consts_free(vm_Consts_ptr);

#endif
