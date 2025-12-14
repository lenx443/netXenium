#include "implement.h"
#include "netXenium.h"

struct Foo_Instance {
  Xen_INSTANCE_HEAD;
};

Xen_ImplementStruct Foo = {.__impl_name = "Foo",
                           .__inst_size = sizeof(struct Foo_Instance)};

Xen_ImplementStruct* implements[] = {&Foo, Xen_NULL};

struct Xen_Module_Def* Xen_Module_io_Start(void*);
struct Xen_Module_Def* Xen_Module_io_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("io", Xen_NULL, Xen_NULL, implements);
}
