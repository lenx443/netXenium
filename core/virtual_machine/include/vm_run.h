#ifndef __VM_RUN_H__
#define __VM_RUN_H__

#include "instance.h"
#include "xen_typedefs.h"

typedef struct {
  Xen_size_t ctx_id;
  Xen_Instance* retval;
  Xen_bool_t halt : 1;
} VM_Run;

Xen_Instance* vm_run(Xen_size_t);
Xen_Instance* vm_run_top(void);

#endif
