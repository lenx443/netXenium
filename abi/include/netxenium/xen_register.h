#ifndef __XEN_REGISTER_H__
#define __XEN_REGISTER_H__

#include "instance.h"
#include "run_ctx.h"

int xen_register_prop_set(const char*, Xen_Instance*, ctx_id_t);
Xen_Instance* xen_register_prop_get(const char*, ctx_id_t);

#endif
