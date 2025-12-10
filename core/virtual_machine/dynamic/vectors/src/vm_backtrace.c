#include <stddef.h>

#include "source_file.h"
#include "vm_backtrace.h"
#include "xen_alloc.h"
#include "xen_typedefs.h"

vm_backtrace* vm_backtrace_new(void) {
  vm_backtrace* bt = Xen_Alloc(sizeof(vm_backtrace));
  bt->bt_addrs = NULL;
  bt->bt_count = 0;
  bt->bt_cap = 0;
  return bt;
}

void vm_backtrace_push(vm_backtrace* bt, Xen_Source_Address sta) {
  if (bt->bt_count >= bt->bt_cap) {
    Xen_size_t new_cap = (bt->bt_cap == 0) ? 4 : bt->bt_cap * 2;
    bt->bt_addrs =
        Xen_Realloc(bt->bt_addrs, new_cap * sizeof(Xen_Source_Address));
    bt->bt_cap = new_cap;
  }
  bt->bt_addrs[bt->bt_count++] = sta;
}

void vm_backtrace_clear(vm_backtrace* bt) {
  Xen_Dealloc(bt->bt_addrs);
  bt->bt_addrs = NULL;
  bt->bt_count = 0;
  bt->bt_cap = 0;
}

void vm_backtrace_free(vm_backtrace* bt) {
  Xen_Dealloc(bt->bt_addrs);
  Xen_Dealloc(bt);
}
