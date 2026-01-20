#include "net_tcp.h"
#include "net_endpoint.h"
#include "net_ip.h"

#include <string.h>

extern Xen_Implement* SOCKET_IMPLEMENT;

Xen_Instance* Net_TCP_Server(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Function_ArgSpec args_def[] = {
      {"endpoint", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* endpoint_inst =
      Xen_Function_ArgBinding_Search(binding, "endpoint")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(endpoint_inst) != ENDPOINT_IMPLEMENT_ptr) {
    return NULL;
  }
  Xen_Instance* sock = NULL;
  IP* ip = (IP*)Net_EndPoint_IP(endpoint_inst);
  Xen_uint16_t port = Net_EndPoint_Port(endpoint_inst);
  if (ip->ip_type == NET_IPV4) {
    sock = Xen_Create(SOCKET_IMPLEMENT, nil, nil);
    Xen_IGC_Push(sock);
    struct sockaddr_in sin;
    memcpy(&sin.sin_addr, &ip->ipv4, sizeof(sin.sin_addr));
    sin.sin_port = port;
    sin.sin_family = AF_INET;
    Xen_Instance* sin_bytes =
        Xen_Bytes_From_Array(sizeof(sin), (Xen_uint8_t*)&sin);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin_bytes);
    Xen_Method_Attr_Str_Call(sock, "bind", bind_args, nil);
  } else if (ip->ip_type == NET_IPV6) {
    Xen_Instance* arg_family = Xen_Number_From_Int(AF_INET6);
    Xen_Instance* sock_args = Xen_Tuple_From_Array(1, &arg_family);
    sock = Xen_Create(SOCKET_IMPLEMENT, sock_args, nil);
    Xen_IGC_Push(sock);
    struct sockaddr_in6 sin6;
    memcpy(&sin6.sin6_addr, &ip->ipv6, sizeof(sin6.sin6_addr));
    sin6.sin6_port = port;
    sin6.sin6_family = AF_INET6;
    Xen_Instance* sin_bytes =
        Xen_Bytes_From_Array(sizeof(sin6), (Xen_uint8_t*)&sin6);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin_bytes);
    Xen_Method_Attr_Str_Call(sock, "bind", bind_args, nil);
  } else {
    return NULL;
  }
  Xen_Method_Attr_Str_Call(sock, "listen", nil, nil);
  Xen_IGC_Pop();
  return sock;
}

Xen_Instance* Net_TCP_Client(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Function_ArgSpec args_def[] = {
      {"endpoint", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* endpoint_inst =
      Xen_Function_ArgBinding_Search(binding, "endpoint")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(endpoint_inst) != ENDPOINT_IMPLEMENT_ptr) {
    return NULL;
  }
  Xen_Instance* sock = NULL;
  IP* ip = (IP*)Net_EndPoint_IP(endpoint_inst);
  Xen_uint16_t port = Net_EndPoint_Port(endpoint_inst);
  if (ip->ip_type == NET_IPV4) {
    sock = Xen_Create(SOCKET_IMPLEMENT, nil, nil);
    Xen_IGC_Push(sock);
    struct sockaddr_in sin;
    memcpy(&sin.sin_addr, &ip->ipv4, sizeof(sin.sin_addr));
    sin.sin_port = port;
    sin.sin_family = AF_INET;
    Xen_Instance* sin_bytes =
        Xen_Bytes_From_Array(sizeof(sin), (Xen_uint8_t*)&sin);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin_bytes);
    Xen_Method_Attr_Str_Call(sock, "connect", bind_args, nil);
  } else if (ip->ip_type == NET_IPV6) {
    Xen_Instance* arg_family = Xen_Number_From_Int(AF_INET6);
    Xen_Instance* sock_args = Xen_Tuple_From_Array(1, &arg_family);
    sock = Xen_Create(SOCKET_IMPLEMENT, sock_args, nil);
    Xen_IGC_Push(sock);
    struct sockaddr_in6 sin6;
    memcpy(&sin6.sin6_addr, &ip->ipv6, sizeof(sin6.sin6_addr));
    sin6.sin6_port = port;
    sin6.sin6_family = AF_INET6;
    Xen_Instance* sin_bytes =
        Xen_Bytes_From_Array(sizeof(sin6), (Xen_uint8_t*)&sin6);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin_bytes);
    Xen_Method_Attr_Str_Call(sock, "connect", bind_args, nil);
  } else {
    return NULL;
  }
  Xen_IGC_Pop();
  return sock;
}
