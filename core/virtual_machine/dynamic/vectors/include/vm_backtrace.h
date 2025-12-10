#ifndef __VM_BACKTRACE_H__
#define __VM_BACKTRACE_H__

#include "source_file.h"
#include "xen_typedefs.h"

struct __backtrace {
  Xen_Source_Address* bt_addrs;
  Xen_size_t bt_count;
  Xen_size_t bt_cap;
};

typedef struct __backtrace vm_backtrace;

vm_backtrace* vm_backtrace_new(void);
void vm_backtrace_push(vm_backtrace*, Xen_Source_Address);
void vm_backtrace_clear(vm_backtrace*);
void vm_backtrace_free(vm_backtrace*);

#endif
