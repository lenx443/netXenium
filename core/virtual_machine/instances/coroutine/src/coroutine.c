#include "coroutine.h"
#include "coroutine_instance.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"

Xen_Instance* Xen_Coroutine_New(Xen_Instance* context) {
  Xen_Coroutine* coro = (Xen_Coroutine*)__instance_new(xen_globals->implements->coroutine, nil, nil, 0);
  Xen_GC_Write_Field(
    (struct __GC_Header *)coro,
    (struct __GC_Handle **)&coro->context,
    (struct __GC_Header *)context);
  Xen_Ctx_Enable_End(context);
  coro->status = Xen_CORO_CREATED;
  return (Xen_Instance*)coro;
}
