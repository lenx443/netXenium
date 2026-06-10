#include "net_endpoint.h"
#include "net_ip.h"
#include "net_tcp.h"
#include "netxenium/netXenium.h"

extern Xen_Instance* MODULE_SOCKETS;
extern Xen_Implement* SOCKET_IMPLEMENT;
extern Xen_Implement* ASOCKET_IMPLEMENT;
Xen_Instance* MODULE_SOCKETS = NULL;
Xen_Implement* SOCKET_IMPLEMENT = NULL;
Xen_Implement* ASOCKET_IMPLEMENT = NULL;

static Xen_Instance* Net_Init(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  MODULE_SOCKETS = Xen_Load("sockets");
  SOCKET_IMPLEMENT = (Xen_Implement*)Xen_Attr_Get_Str(MODULE_SOCKETS, "Socket");
  ASOCKET_IMPLEMENT = (Xen_Implement*)Xen_Attr_Get_Str(MODULE_SOCKETS, "ASocket");
  IP_init(self);
  EndPoint_init(self);
  return nil;
}

struct Xen_Module_Function functions[] = {
    {"TCPServer", Net_TCP_Server},
    {"TCPClient", Net_TCP_Client},
    {NULL, NULL},
};

struct Xen_Module_Function_Async functions_async[] = {
    {"TCPAServer", Net_TCP_AServer, 0},
    {"TCPAClient", Net_TCP_AClient, sizeof(struct __Net_TCP_AClient_Status)},
    {NULL, NULL, 0},
};

static Xen_ImplementStruct* implements[] = {
    &IP_implmenet,
    &EndPoint_implmenet,
    NULL,
};

struct Xen_Module_Def* Xen_Module_net_Start(void*);
struct Xen_Module_Def* Xen_Module_net_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("net", Net_Init, functions, functions_async, implements);
}
