#include "xen_queue_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_function.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_queue.h"
#include "xen_queue_instance.h"
#include "xen_typedefs.h"

static void queue_trace(Xen_Instance* inst) {
  Xen_Queue* queue = (Xen_Queue*)inst;
  for (Xen_size_t i = 0; i < queue->size; i++) {
    Xen_size_t idx = (queue->head + i) & (queue->capacity - 1);
    Xen_GC_Trace_GCHeader(queue->buf[idx]);
  }
}

static Xen_Instance *queue_destroy(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  while (Xen_Queue_Pop(self) != NULL);
  Xen_Dealloc(((Xen_Queue*)self)->buf);
  return nil;
}

static Xen_Instance *queue_push(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ArgSpec args_def[] = {
      {"value", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* value =
      Xen_Function_ArgBinding_Search(binding, "value")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Queue_Push(self, value);
  return nil;
}

static Xen_Instance *queue_pop(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Instance* value = Xen_Queue_Pop(self);
  if (!value) {
    Xen_VM_Except_Throw(Xen_Except_New("QueueEmpty", NULL));
    return NULL;
  }
  return value;
}

static struct __Implement __Queue_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Queue",
    .__inst_size = sizeof(struct Xen_Queue_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = queue_trace,
    .__props = NULL,
    .__alloc = NULL,
    .__create = NULL,
    .__destroy = queue_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_Queue_GetImplement(void) {
  return &__Queue_Implement;
}

int Xen_Queue_Init(void) {
  if (!Xen_VM_Store_Global("queue", (Xen_Instance*)xen_globals->implements->queue)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  Xen_VM_Store_Native_Function(props, "push", queue_push, nil);
  Xen_VM_Store_Native_Function(props, "pop", queue_pop, nil);
  __Queue_Implement.__props =
      Xen_GCHandle_New_From((Xen_GCHeader*)impls_maps, (Xen_GCHeader*)props);
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Queue_Finish(void) {
  Xen_GCHandle_Free(__Queue_Implement.__props);
}
