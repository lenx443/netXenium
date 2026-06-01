#include "asocket.h"
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "socket.h"
#include "netxenium/implement.h"
#include "netxenium/netXenium.h"

static Xen_Instance* sockets_getaddrinfo(Xen_Instance* self, Xen_Instance* args,
                                         Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Function_ArgSpec args_def[] = {
      {"node", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, Xen_NULL},
      {"service", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {"family", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {"socktype", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {"proto", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {"flags", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_c_string_t node = Xen_String_As_CString(
      Xen_Function_ArgBinding_Search(binding, "node")->value);
  Xen_Function_ArgBound* service_arg =
      Xen_Function_ArgBinding_Search(binding, "service");
  Xen_c_string_t service = NULL;
  if (service_arg->provided) {
    service = Xen_String_As_CString(service_arg->value);
  }
  Xen_Function_ArgBound* family_arg =
      Xen_Function_ArgBinding_Search(binding, "family");
  int family = 0;
  if (family_arg->provided) {
    family = Xen_Number_As_Int(family_arg->value);
  }
  Xen_Function_ArgBound* socktype_arg =
      Xen_Function_ArgBinding_Search(binding, "socktype");
  int socktype = 0;
  if (socktype_arg->provided) {
    socktype = Xen_Number_As_Int(socktype_arg->value);
  }
  Xen_Function_ArgBound* proto_arg =
      Xen_Function_ArgBinding_Search(binding, "proto");
  int proto = 0;
  if (proto_arg->provided) {
    proto = Xen_Number_As_Int(proto_arg->value);
  }
  Xen_Function_ArgBound* flags_arg =
      Xen_Function_ArgBinding_Search(binding, "flags");
  int flags = 0;
  if (flags_arg->provided) {
    flags = Xen_Number_As_Int(flags_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  struct addrinfo* res = NULL;
  if (service) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = proto;
    hints.ai_flags = flags;
    if (getaddrinfo(node, service, &hints, &res) != 0) {
      return NULL;
    }
  } else {
    if (getaddrinfo(node, NULL, NULL, &res) != 0) {
      return NULL;
    }
  }
  Xen_Instance* result = Xen_Vector_New();
  for (struct addrinfo* p = res; p != NULL; p = p->ai_next) {
    Xen_Instance* rflags = Xen_Number_From_Int(p->ai_flags);
    Xen_Instance* rfamily = Xen_Number_From_Int(p->ai_family);
    Xen_Instance* rsocktype = Xen_Number_From_Int(p->ai_socktype);
    Xen_Instance* rproto = Xen_Number_From_Int(p->ai_protocol);
    Xen_Instance* raddr = NULL;
    if (p->ai_family == AF_INET) {
      char buffer[INET_ADDRSTRLEN];
      if (!inet_ntop(p->ai_family, &((struct sockaddr_in*)p->ai_addr)->sin_addr,
                     buffer, sizeof(buffer))) {
        freeaddrinfo(res);
        return NULL;
      }
      raddr = Xen_String_From_CString(buffer);
    } else if (p->ai_family == AF_INET6) {
      char buffer[INET6_ADDRSTRLEN];
      if (!inet_ntop(p->ai_family, &((struct sockaddr_in6*)p->ai_addr)->sin6_addr, buffer, sizeof(buffer))) {
        freeaddrinfo(res);
        return NULL;
      }
      raddr = Xen_String_From_CString(buffer);
    } else {
      freeaddrinfo(res);
      return NULL;
    }
    Xen_Instance* rcanonname = NULL;
    if (p->ai_canonname) {
      rcanonname = Xen_String_From_CString(p->ai_canonname);
    } else {
      rcanonname = Xen_String_From_CString("");
    }
    Xen_Instance* rtuple = Xen_Tuple_From_Array(6, (Xen_Instance*[]){
        rflags, rfamily, rsocktype, rproto, raddr, rcanonname
    });
    Xen_Vector_Push(result, rtuple);
  }
  freeaddrinfo(res);
  return result;
}

static Xen_Instance* Sockets_Init(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Socket_init(self);
  ASocket_Init(self);
  Xen_Attr_Set_Str(self, "AF_INET", Xen_Number_From_Int(AF_INET));
  Xen_Attr_Set_Str(self, "AF_INET6", Xen_Number_From_Int(AF_INET6));
  Xen_Attr_Set_Str(self, "SOCK_STREAM", Xen_Number_From_Int(SOCK_STREAM));
  Xen_Attr_Set_Str(self, "SOCK_DGRAM", Xen_Number_From_Int(SOCK_DGRAM));
  Xen_Attr_Set_Str(self, "SOCK_RAW", Xen_Number_From_Int(SOCK_RAW));
  Xen_Attr_Set_Str(self, "IPPROTO_TCP", Xen_Number_From_Int(IPPROTO_TCP));
  Xen_Attr_Set_Str(self, "IPPROTO_UDP", Xen_Number_From_Int(IPPROTO_UDP));
  Xen_Attr_Set_Str(self, "IPPROTO_ICMP", Xen_Number_From_Int(IPPROTO_ICMP));
  Xen_Attr_Set_Str(self, "IPPROTO_ICMPV6", Xen_Number_From_Int(IPPROTO_ICMPV6));
  Xen_Attr_Set_Str(self, "SHUT_RD", Xen_Number_From_Int(SHUT_RD));
  Xen_Attr_Set_Str(self, "SHUT_WR", Xen_Number_From_Int(SHUT_WR));
  Xen_Attr_Set_Str(self, "SHUT_RDWR", Xen_Number_From_Int(SHUT_RDWR));
  return nil;
}

struct Xen_Module_Function functions[] = {
    {"getaddrinfo", sockets_getaddrinfo},
    {NULL, NULL},
};

static Xen_ImplementStruct* implements[] = {
    &Socket_Implement,
    &ASocket_Implement,
    NULL,
};

struct Xen_Module_Def* Xen_Module_sockets_Start(void*);
struct Xen_Module_Def* Xen_Module_sockets_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("sockets", Sockets_Init, functions, NULL, implements);
}
