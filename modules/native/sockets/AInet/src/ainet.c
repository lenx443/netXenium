#include "ainet.h"
#include "netxenium/netXenium.h"
#include <arpa/inet.h>
#include <linux/in.h>
#include <netinet/in.h>
#include <sys/socket.h>

Xen_Implement *AInet_Implement_Pointer = NULL;

static Xen_Instance *ainet_create(Xen_Instance *self, Xen_Instance *args,
                                  Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  AInet *ain = (AInet *)self;
  Xen_Function_ArgSpec args_def[] = {
      {"addr", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, Xen_NULL},
      {"port", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, Xen_NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) { return NULL; }
  Xen_c_string_t addr =
      Xen_String_As_CString(Xen_Function_ArgBinding_Search(binding, "addr")->value);
  Xen_uint16_t port =
      Xen_Number_As_UInt(Xen_Function_ArgBinding_Search(binding, "port")->value);
  Xen_Function_ArgBinding_Free(binding);
  if (inet_pton(AF_INET, addr, &ain->addr) != 1) { return NULL; }
  ain->port = htons(port);
  return nil;
}

static Xen_Instance *ainet_addr(Xen_Instance *self, Xen_Instance *args,
                                Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  AInet *ain = (AInet *)self;
  char r[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &ain->addr, r, INET_ADDRSTRLEN) == NULL) { return NULL; };
  r[INET_ADDRSTRLEN - 1] = '\0';
  return Xen_String_From_CString(r);
}

static Xen_Instance *ainet_port(Xen_Instance *self, Xen_Instance *args,
                                Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  AInet *ain = (AInet *)self;
  return Xen_Number_From_UInt(ain->port);
}

Xen_ImplementStruct AInet_Implement = {
    .__impl_name = "AInet",
    .__inst_size = sizeof(AInet),
    .__create = ainet_create,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

void AInet_Init(Xen_Instance *module) {
  AInet_Implement_Pointer = (Xen_Implement *)Xen_Attr_Get_Str(module, "AInet");
  Xen_Instance *props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "addr", ainet_addr, nil);
  Xen_VM_Store_Native_Function(props, "port", ainet_port, nil);
  Xen_Implement_SetProps(AInet_Implement_Pointer, props);
}

Xen_Instance *AInet_From_Struct(struct sockaddr_in *sin) {
  AInet *ain = (AInet *)__instance_new(AInet_Implement_Pointer, nil, nil, 0);
  ain->addr = sin->sin_addr.s_addr;
  ain->port = sin->sin_port;
  return (Xen_Instance *)ain;
}

void AInet_Get(Xen_Instance *inst, struct sockaddr_in *sin) {
  AInet *ain = (AInet *)inst;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = ain->addr;
  sin->sin_port = ain->port;
}
