#include "ainet6.h"
#include "netxenium/netXenium.h"
#include <arpa/inet.h>
#include <linux/in.h>
#include <string.h>
#include <sys/socket.h>

Xen_Implement *AInet6_Implement_Pointer = NULL;

static Xen_Instance *ainet6_create(Xen_Instance *self, Xen_Instance *args,
                                   Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  AInet6 *ain6 = (AInet6 *)self;
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
  if (inet_pton(AF_INET6, addr, &ain6->addr) != 1) { return NULL; }
  ain6->port = htons(port);
  return nil;
}

static Xen_Instance *ainet6_addr(Xen_Instance *self, Xen_Instance *args,
                                 Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  AInet6 *ain6 = (AInet6 *)self;
  char r[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, &ain6->addr, r, INET6_ADDRSTRLEN) == NULL) { return NULL; };
  r[INET6_ADDRSTRLEN - 1] = '\0';
  return Xen_String_From_CString(r);
}

static Xen_Instance *ainet6_port(Xen_Instance *self, Xen_Instance *args,
                                 Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  AInet6 *ain6 = (AInet6 *)self;
  return Xen_Number_From_UInt(ain6->port);
}

Xen_ImplementStruct AInet6_Implement = {
    .__impl_name = "AInet6",
    .__inst_size = sizeof(AInet6),
    .__create = ainet6_create,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

void AInet6_Init(Xen_Instance *module) {
  AInet6_Implement_Pointer = (Xen_Implement *)Xen_Attr_Get_Str(module, "AInet6");
  Xen_Instance *props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "addr", ainet6_addr, nil);
  Xen_VM_Store_Native_Function(props, "port", ainet6_port, nil);
  Xen_Implement_SetProps(AInet6_Implement_Pointer, props);
}

Xen_Instance *AInet6_From_Struct(struct sockaddr_in6 *sin) {
  AInet6 *ain = (AInet6 *)__instance_new(AInet6_Implement_Pointer, nil, nil, 0);
  memcpy(&ain->addr, &sin->sin6_addr, sizeof(struct in6_addr));
  ain->port = sin->sin6_port;
  return (Xen_Instance *)ain;
}

void AInet6_Get(Xen_Instance *inst, struct sockaddr_in6 *sin) {
  AInet6 *ain = (AInet6 *)inst;
  sin->sin6_family = AF_INET6;
  memcpy(&sin->sin6_addr, &ain->addr, sizeof(struct in6_addr));
  sin->sin6_port = ain->port;
}
