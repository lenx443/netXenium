#include "net_ip.h"
#include "netxenium/netXenium.h"

static Xen_Instance* Net_Init(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  IP_init(self);
  return nil;
}

Xen_ImplementStruct* implements[] = {
    &IP_implmenet,
    NULL,
};

struct Xen_Module_Def* Xen_Module_net_Start(void*);
struct Xen_Module_Def* Xen_Module_net_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("net", Net_Init, NULL, implements);
}
