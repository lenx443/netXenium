#include "netXenium.h"

static Xen_Instance* example(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_Number_From_Int(3440);
}

static struct Xen_Module_Function functions[] = {
    {"example", example},
    {Xen_NULL, Xen_NULL},
};

struct Xen_Module_Def* Xen_Module_io_Start(void*);
struct Xen_Module_Def* Xen_Module_io_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("io", functions);
}
