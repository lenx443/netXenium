#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "netxenium/netXenium.h"

#define SOCKET_AF_INET 1

#define SOCKET_SOCK_STREAM 1

#define SOCKET_CAP_READ (1 << 0)
#define SOCKET_CAP_WRITE (1 << 1)
#define SOCKET_CAP_BIND (1 << 2)

static Xen_Implement* Socket_Implememnt_Pointer = NULL;

typedef struct {
  Xen_INSTANCE_HEAD;
  int f;
  Xen_bool_t open;
  int domain;
  int type;
  int protocol;
  struct sockaddr_in local;
  struct sockaddr_in remote;
  Xen_uint32_t caps;
} Socket;

static Xen_Instance* socket_create(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"family", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_Number_From_Int(SOCKET_AF_INET)},
      {"type", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_Number_From_Int(SOCKET_SOCK_STREAM)},
      {"proto", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_Number_From_Int(0)},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int domain_arg = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "family")->value);
  int type_arg =
      Xen_Number_As_Int(Xen_Function_ArgBinding_Search(binding, "type")->value);
  int protocol = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "proto")->value);
  Xen_Function_ArgBinding_Free(binding);
  int domain;
  switch (domain_arg) {
  case SOCKET_AF_INET:
    domain = AF_INET;
    break;
  default:
    return Xen_NULL;
  }
  int type;
  switch (type_arg) {
  case SOCKET_SOCK_STREAM:
    type = SOCK_STREAM;
    break;
  default:
    return Xen_NULL;
  }
  Socket* sock = (Socket*)self;
  sock->domain = domain;
  sock->type = type;
  sock->protocol = protocol;
  sock->f = socket(domain, type, protocol);
  if (sock->f < 0) {
    return Xen_NULL;
  }
  sock->open = 1;
  sock->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
  return nil;
}

static Xen_Instance* socket_destroy(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Socket* sock = (Socket*)self;
  if (sock->open) {
    close(sock->f);
    sock->open = 0;
  }
  return nil;
}

static Xen_Instance* socket_bind(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"ip", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"port", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_c_string_t ip = Xen_String_As_CString(
      Xen_Function_ArgBinding_Search(binding, "ip")->value);
  Xen_uint16_t port = Xen_Number_As_UInt(
      Xen_Function_ArgBinding_Search(binding, "port")->value);
  Xen_Function_ArgBinding_Free(binding);
  Socket* sock = (Socket*)self;
  memset(&sock->local, 0, sizeof(sock->local));
  sock->local.sin_family = sock->domain;
  sock->local.sin_port = htons(port);
  if (inet_pton(sock->domain, ip, &sock->local.sin_addr) != 1) {
    return NULL;
  }
  if (bind(sock->f, (struct sockaddr*)&sock->local, sizeof(sock->local)) < 0) {
    return NULL;
  }
  sock->caps |= SOCKET_CAP_BIND;
  return nil;
}

static Xen_Instance* socket_close(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpy(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (sock->open) {
    close(sock->f);
    sock->open = 0;
  }
  return nil;
}

static Xen_ImplementStruct Socket_Implememnt = {
    .__impl_name = "Socket",
    .__inst_size = sizeof(Socket),
    .__create = socket_create,
    .__destroy = socket_destroy,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

static Xen_Instance* Sockets_Init(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if ((Socket_Implememnt_Pointer =
           (Xen_Implement*)Xen_Attr_Get_Str(self, "Socket")) == Xen_NULL) {
    return Xen_NULL;
  }
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "bind", socket_bind, nil);
  Xen_VM_Store_Native_Function(props, "close", socket_close, nil);
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"AF_INET", Xen_Number_From_Int(SOCKET_AF_INET)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SOCK_STREAM",
                                Xen_Number_From_Int(SOCKET_SOCK_STREAM)});
  Xen_Implement_SetProps(Socket_Implememnt_Pointer, props);
  return nil;
}

static Xen_ImplementStruct* implements[] = {
    &Socket_Implememnt,
    NULL,
};

struct Xen_Module_Def* Xen_Module_sockets_Start(void*);
struct Xen_Module_Def* Xen_Module_sockets_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("sockets", Sockets_Init, Xen_NULL, implements);
}
