#include "net_tcp.h"
#include "net_endpoint.h"
#include "net_ip.h"

#include <string.h>

extern Xen_Implement* SOCKET_IMPLEMENT;
extern Xen_Implement* ASOCKET_IMPLEMENT;

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
    Xen_Instance* sin6_bytes =
        Xen_Bytes_From_Array(sizeof(sin6), (Xen_uint8_t*)&sin6);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin6_bytes);
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

void Net_TCP_AServer(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_ASYNC_CLEAR_ARG_NEVER_USE;
  Xen_Function_ArgSpec args_def[] = {
      {"endpoint", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    Xen_COROUTINE_EXCEPTED;
  }
  Xen_Instance* endpoint_inst =
      Xen_Function_ArgBinding_Search(binding, "endpoint")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(endpoint_inst) != ENDPOINT_IMPLEMENT_ptr) {
    Xen_CallError(coro);
    Xen_COROUTINE_EXCEPTED;
  }
  Xen_Instance* sock = NULL;
  IP* ip = (IP*)Net_EndPoint_IP(endpoint_inst);
  Xen_uint16_t port = Net_EndPoint_Port(endpoint_inst);
  if (ip->ip_type == NET_IPV4) {
    sock = Xen_Create(ASOCKET_IMPLEMENT, nil, nil);
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
    sock = Xen_Create(ASOCKET_IMPLEMENT, sock_args, nil);
    Xen_IGC_Push(sock);
    struct sockaddr_in6 sin6;
    memcpy(&sin6.sin6_addr, &ip->ipv6, sizeof(sin6.sin6_addr));
    sin6.sin6_port = port;
    sin6.sin6_family = AF_INET6;
    Xen_Instance* sin6_bytes =
        Xen_Bytes_From_Array(sizeof(sin6), (Xen_uint8_t*)&sin6);
    Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sin6_bytes);
    Xen_Method_Attr_Str_Call(sock, "bind", bind_args, nil);
  } else {
    Xen_CallError(coro);
    Xen_COROUTINE_EXCEPTED;
  }
  Xen_Method_Attr_Str_Call(sock, "listen", nil, nil);
  Xen_IGC_Pop();
  Xen_COROUTINE_RETURN(sock);
}

void Net_TCP_AClient(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_ASYNC_CLEAR_ARG_NEVER_USE;
  struct __Net_TCP_AClient_Status* status = Xen_Coroutine_Data(coro);
  Xen_IGC_Fork* roots = Xen_Coroutine_IGC_Fork(coro);
  switch (status->step) {
  case 0: {
  Xen_Function_ArgSpec args_def[] = {
      {"endpoint", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    Xen_COROUTINE_EXCEPTED;
  }
  Xen_Instance* endpoint_inst =
      Xen_Function_ArgBinding_Search(binding, "endpoint")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(endpoint_inst) != ENDPOINT_IMPLEMENT_ptr) {
    Xen_CallError(coro);
    Xen_COROUTINE_EXCEPTED;
  }
  IP* ip = (IP*)Net_EndPoint_IP(endpoint_inst);
  if (ip->ip_type == NET_IPV4) {
    status->sock = Xen_Create(ASOCKET_IMPLEMENT, nil, nil);
    Xen_IGC_Fork_Push(roots, status->sock);
  } else if (ip->ip_type == NET_IPV6) {
    Xen_Instance* arg_family = Xen_Number_From_Int(AF_INET6);
    Xen_Instance* sock_args = Xen_Tuple_From_Array(1, &arg_family);
    status->sock = Xen_Create(ASOCKET_IMPLEMENT, sock_args, nil);
    Xen_IGC_Fork_Push(roots, status->sock);
  } else {
    Xen_CallError(coro);
    Xen_COROUTINE_EXCEPTED;
  }
  Xen_Instance* sock_bytes = Xen_Method_Attr_Str_Call(endpoint_inst, "bytes", nil, nil);
  Xen_Instance* bind_args = Xen_Tuple_From_Array(1, &sock_bytes);
  if (!Xen_Coroutine_Await(
    coro,
    Xen_Method_Attr_Str_Call(status->sock, "connect", bind_args, nil))) {
    Xen_COROUTINE_EXCEPTED;
  }
  status->step++;
  return;
  }
  case 1:
  Xen_IGC_Fork_Pop(roots);
  Xen_COROUTINE_RETURN(status->sock);
  }
}
