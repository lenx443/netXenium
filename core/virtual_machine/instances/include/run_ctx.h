#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include "instance.h"

#define CTX_FLAG_STATIC (1 << 0)
#define CTX_FLAG_PROPS (1 << 1)

#define CTX_GET_FLAG(ctx, flag)                                                \
  (((((struct RunContext*)ctx)->ctx_flags) & (flag)) != 0)

typedef Xen_ulong_t ctx_id_t;

Xen_Instance* Xen_Ctx_New(Xen_Instance*, Xen_Instance*, Xen_Instance*,
                          Xen_Instance*, Xen_Instance*, Xen_Instance*);

ctx_id_t run_ctx_id(Xen_Instance*);

#endif
