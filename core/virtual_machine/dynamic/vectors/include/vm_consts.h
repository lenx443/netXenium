#ifndef __VM_CONSTS_H__
#define __VM_CONSTS_H__

#include <stdbool.h>
#include <stddef.h>

#include "xen_typedefs.h"

struct __Instance;

typedef struct {
  struct __Instance* c_names;
  struct __Instance* c_instances;
} vm_Consts;

typedef vm_Consts* vm_Consts_ptr;

vm_Consts_ptr vm_consts_new();
vm_Consts_ptr vm_consts_from_values(struct __Instance*, struct __Instance*);
Xen_ssize_t vm_consts_push_name(vm_Consts_ptr, const char*);
Xen_ssize_t vm_consts_push_instance(vm_Consts_ptr, struct __Instance*);
void vm_consts_free(vm_Consts_ptr);

#endif
