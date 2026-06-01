#include "asocket.h"
#include "netxenium/coroutine.h"
#include "socket.h"
#include "netxenium/netXenium.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

Xen_Implement* ASocket_Implement_Pointer = NULL;

static void asocket_trace(Xen_Instance* inst) {
  ASocket* sock = (ASocket*)inst;
  if (sock->f->ptr) Xen_GC_Trace_GCHeader(sock->f);
}

static Xen_Instance* asocket_alloc(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  ASocket* sock = (ASocket*)Xen_Instance_Alloc(ASocket_Implement_Pointer);
  sock->f = Xen_GCHandle_New((Xen_GCHeader*)sock);
  return (Xen_Instance*)sock;
}

static Xen_Instance* asocket_create(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"family", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {"type", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {"proto", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Function_ArgBound* domain_arg =
      Xen_Function_ArgBinding_Search(binding, "family");
  int domain = AF_INET;
  if (domain_arg->provided) {
    domain = Xen_Number_As_Int(domain_arg->value);
  }
  Xen_Function_ArgBound* type_arg =
      Xen_Function_ArgBinding_Search(binding, "type");
  int type = SOCK_STREAM;
  if (type_arg->provided) {
    type = Xen_Number_As_Int(type_arg->value);
  }
  Xen_Function_ArgBound* protocol_arg =
      Xen_Function_ArgBinding_Search(binding, "proto");
  int protocol = 0;
  if (protocol_arg->provided) {
    protocol = Xen_Number_As_Int(protocol_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  ASocket* sock = (ASocket*)self;
  sock->domain = domain;
  sock->type = type;
  sock->protocol = protocol;
  int f = socket(domain, type, protocol);
  if (f < 0) {
    return Xen_NULL;
  }
  Xen_IO_Status* io = Xen_IO_Status_New(&f);
  if (!io) {
    close(f);
    return NULL;
  }
  Xen_GC_Write_Field(&sock->f, (Xen_GCHeader*)io);
  sock->open = 1;
  sock->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
  Xen_Method_Attr_Str_Call(
    (Xen_Instance*)sock,
    "set_nonblock",
    Xen_Tuple_From_Array(1, (Xen_Instance*[]){Xen_True}),
    nil
  );
  return nil;
}

static Xen_Instance* asocket_destroy(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  ASocket* sock = (ASocket*)self;
  if (sock->open) {
    Xen_IO_Status_Close((Xen_IO_Status*)sock->f->ptr);
    sock->open = 0;
  }
  Xen_GCHandle_Free(sock->f);
  return nil;
}

static Xen_Instance* asocket_bind(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"addr", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* addr = Xen_Function_ArgBinding_Search(binding, "addr")->value;
  Xen_Function_ArgBinding_Free(binding);
  switch (sock->domain) {
  case AF_INET: {
    if (Xen_IsTuple(addr)) {
      struct Socket_Address_IP address;
      if (!Socket_Addr_IP_Get(addr, &address)) {
        return NULL;
      }
      memset(&sock->local.ipv4, 0, sizeof(sock->local.ipv4));
      sock->local.ipv4.sin_family = sock->domain;
      sock->local.ipv4.sin_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &sock->local.ipv4.sin_addr) !=
          1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(sock->local.ipv4)) {
        return NULL;
      }
      memcpy(&sock->local.ipv4, Xen_Bytes_Get(addr), sizeof(sock->local.ipv4));
    } else {
      return NULL;
    }
    if (bind(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
             (struct sockaddr*)&sock->local.ipv4,
             sizeof(sock->local.ipv4)) < 0) {
      return NULL;
    }
    break;
  }
  case AF_INET6: {
    if (Xen_IsTuple(addr)) {
      struct Socket_Address_IP address;
      if (!Socket_Addr_IP_Get(addr, &address)) {
        return NULL;
      }
      memset(&sock->local.ipv6, 0, sizeof(sock->local.ipv6));
      sock->local.ipv6.sin6_family = sock->domain;
      sock->local.ipv6.sin6_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &sock->local.ipv6.sin6_addr) !=
          1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(sock->local.ipv6)) {
        return NULL;
      }
      memcpy(&sock->local.ipv6, Xen_Bytes_Get(addr), sizeof(sock->local.ipv6));
    } else {
      return NULL;
    }
    if (bind(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
             (struct sockaddr*)&sock->local.ipv6,
             sizeof(sock->local.ipv6)) < 0) {
      return NULL;
    }
    break;
  }
  default:
    return NULL;
  }
  sock->caps |= SOCKET_CAP_BIND;
  return nil;
}

static Xen_Instance* asocket_listen(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (sock->type != SOCK_STREAM) {
    return NULL;
  }
  if (!(sock->caps & SOCKET_CAP_BIND)) {
    return NULL;
  }
  if (sock->caps & SOCKET_CAP_CONNECT) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"backlog", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int backlog = 128;
  Xen_Function_ArgBound* backlog_arg =
      Xen_Function_ArgBinding_Search(binding, "backlog");
  if (backlog_arg->provided) {
    backlog = Xen_Number_As_Int(backlog_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  if (listen(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), backlog) < 0) {
    return NULL;
  }
  sock->caps |= SOCKET_CAP_LISTEN | SOCKET_CAP_ACCEPT;
  sock->caps &= ~(SOCKET_CAP_READ | SOCKET_CAP_WRITE);
  return nil;
}

static Xen_Instance* asocket_shutdown(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"how", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int how =
      Xen_Number_As_Int(Xen_Function_ArgBinding_Search(binding, "how")->value);
  Xen_Function_ArgBinding_Free(binding);
  if (how < 0 || how > 2) {
    return NULL;
  }
  if (shutdown(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), how) < 0) {
    return NULL;
  }
  if (how == SHUT_RD) {
    sock->caps &= ~SOCKET_CAP_READ;
  } else if (how == SHUT_WR) {
    sock->caps &= ~SOCKET_CAP_WRITE;
  } else if (how == SHUT_RDWR) {
    sock->caps &= ~(SOCKET_CAP_READ | SOCKET_CAP_WRITE);
  }
  return nil;
}

static Xen_Instance* asocket_setsockopt(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"level", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"optname", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"value", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int level = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "level")->value);
  int optname = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "optname")->value);
  Xen_Instance* value = Xen_Function_ArgBinding_Search(binding, "value")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (setsockopt(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), level, optname, Xen_Bytes_Get(value),
                 Xen_SIZE(value)) < 0) {
    return NULL;
  }
  return nil;
}

static Xen_Instance* asocket_getsockopt(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"level", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"optname", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"buflen", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int level = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "level")->value);
  int optname = Xen_Number_As_Int(
      Xen_Function_ArgBinding_Search(binding, "optname")->value);
  socklen_t buflen = sizeof(int);
  Xen_Function_ArgBound* buflen_bound =
      Xen_Function_ArgBinding_Search(binding, "buflen");
  if (buflen_bound->provided) {
    buflen = Xen_Number_As_Int(buflen_bound->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  Xen_uint8_t* buffer = Xen_ZAlloc(buflen, 1);
  if (getsockopt(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), level, optname, buffer, &buflen) < 0) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Instance* bytes = Xen_Bytes_From_Array(buflen, buffer);
  Xen_Dealloc(buffer);
  return bytes;
}

static Xen_Instance* asocket_set_nonblocking(Xen_Instance* self,
                                            Xen_Instance* args,
                                            Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"nonblock", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_BOOLEAN, XEN_FUNCTION_ARG_OPTIONAL, Xen_True},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* nonblock =
      Xen_Function_ArgBinding_Search(binding, "nonblock")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (nonblock == Xen_True) {
    int flags = fcntl(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), F_GETFL, 0);
    if (fcntl(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), F_SETFL, flags | O_NONBLOCK) == -1) {
      return NULL;
    }
    sock->caps |= SOCKET_CAP_NONBLOCK;
  } else {
    int flags = fcntl(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if (fcntl(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), F_SETFL, flags) == -1) {
      return NULL;
    }
    sock->caps &= ~SOCKET_CAP_NONBLOCK;
  }
  return nil;
}

static Xen_Instance* asocket_getsockname(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getsockname(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&local, &len) < 0) {
      return NULL;
    }
    char ip_str[INET_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &local.sin_addr, ip_str, sizeof(ip_str))) {
      return NULL;
    }
    Xen_Instance* addr =
        Socket_Addr_IP_Set((struct Socket_Address_IP){ip_str, local.sin_port});
    return addr;
  }
  case AF_INET6: {
    struct sockaddr_in6 local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getsockname(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&local, &len) < 0) {
      return NULL;
    }
    char ip_str[INET6_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &local.sin6_addr, ip_str, sizeof(ip_str))) {
      return NULL;
    }
    Xen_Instance* addr =
        Socket_Addr_IP_Set((struct Socket_Address_IP){ip_str, local.sin6_port});
    return addr;
  }
  default:
    return NULL;
  }
}

static Xen_Instance* asocket_getpeername(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    return NULL;
  }
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getpeername(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&local, &len) < 0) {
      return NULL;
    }
    char ip_str[INET_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &local.sin_addr, ip_str, sizeof(ip_str))) {
      return NULL;
    }
    Xen_Instance* addr =
        Socket_Addr_IP_Set((struct Socket_Address_IP){ip_str, local.sin_port});
    return addr;
  }
  case AF_INET6: {
    struct sockaddr_in6 local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getpeername(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&local, &len) < 0) {
      return NULL;
    }
    char ip_str[INET6_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &local.sin6_addr, ip_str, sizeof(ip_str))) {
      return NULL;
    }
    Xen_Instance* addr =
        Socket_Addr_IP_Set((struct Socket_Address_IP){ip_str, local.sin6_port});
    return addr;
  }
  default:
    return NULL;
  }
}

static Xen_Instance* asocket_close(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  ASocket* sock = (ASocket*)self;
  if (sock->open) {
    Xen_IO_Status_Close((Xen_IO_Status*)sock->f->ptr);
    sock->open = 0;
  }
  return nil;
}


static void asocket_accept(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  if (!(sock->caps & SOCKET_CAP_ACCEPT)) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in remote;
    socklen_t len = sizeof(remote);
    int client_fd = accept(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&remote, &len);
    if (client_fd >= 0) {
      ASocket* client =
          (ASocket*)__instance_new(ASocket_Implement_Pointer, nil, nil, 0);
      Xen_GC_Write_Field(&client->f, (Xen_GCHeader*)Xen_IO_Status_New(&client_fd));
      client->open = 1;
      client->domain = sock->domain;
      client->type = sock->type;
      client->protocol = sock->protocol;
      client->remote.ipv4 = remote;
      client->local.ipv4 = sock->local.ipv4;
      client->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
      Xen_Method_Attr_Str_Call(
        (Xen_Instance*)client,
        "set_nonblock",
        Xen_Tuple_From_Array(1, (Xen_Instance*[]){Xen_True}),
        nil
      );
      Xen_COROUTINE_RETURN((Xen_Instance*)client);
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      Xen_IO_Status_SIn((Xen_IO_Status*)sock->f->ptr, coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
      return;
    }
    if (errno == EINTR) {
      return;
    }
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  case AF_INET6: {
    struct sockaddr_in6 remote;
    socklen_t len = sizeof(remote);
    int client_fd = accept(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr), (struct sockaddr*)&remote, &len);
    if (client_fd >= 0) {
      ASocket* client =
          (ASocket*)__instance_new(ASocket_Implement_Pointer, nil, nil, 0);
      Xen_GC_Write_Field(&client->f, (Xen_GCHeader*)Xen_IO_Status_New(&client_fd));
      client->open = 1;
      client->domain = sock->domain;
      client->type = sock->type;
      client->protocol = sock->protocol;
      client->remote.ipv6 = remote;
      client->local.ipv6 = sock->local.ipv6;
      client->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
      Xen_Method_Attr_Str_Call(
        (Xen_Instance*)client,
        "set_nonblock",
        Xen_Tuple_From_Array(1, (Xen_Instance*[]){Xen_True}),
        nil
      );
      Xen_COROUTINE_RETURN((Xen_Instance*)client);
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      Xen_IO_Status_SIn((Xen_IO_Status*)sock->f->ptr, coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
      return;
    }
    if (errno == EINTR) {
      return;
    }
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  default:
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
}

static void asocket_connect(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  if (sock->type != SOCK_STREAM) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  if (sock->caps & (SOCKET_CAP_LISTEN | SOCKET_CAP_ACCEPT)) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  int* connecting = Xen_Coroutine_Data(coro);
  if (!(*connecting)) {
    Xen_Function_ArgSpec args_def[] = {
        {"addr", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY,
         XEN_FUNCTION_ARG_REQUIRED, NULL},
        {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
    };

    Xen_Function_ArgBinding* binding =
        Xen_Function_ArgsParse(args, kwargs, args_def);
    if (!binding) {
      Xen_CallError(coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
      return;
    }
    Xen_Instance* addr = Xen_Function_ArgBinding_Search(binding, "addr")->value;
    Xen_Function_ArgBinding_Free(binding);
    switch (sock->domain) {
    case AF_INET: {
      if (Xen_IsTuple(addr)) {
        struct Socket_Address_IP address;
        if (!Socket_Addr_IP_Get(addr, &address)) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
        memset(&sock->remote.ipv4, 0, sizeof(sock->remote.ipv4));
        sock->remote.ipv4.sin_family = sock->domain;
        sock->remote.ipv4.sin_port = htons(address.port);
        if (inet_pton(sock->domain, address.ip, &sock->remote.ipv4.sin_addr) !=
            1) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
      } else if (Xen_IsBytes(addr)) {
        if (Xen_SIZE(addr) != sizeof(sock->remote.ipv4)) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
        memcpy(&sock->remote.ipv4, Xen_Bytes_Get(addr),
               sizeof(sock->remote.ipv4));
      } else {
        Xen_CallError(coro);
        Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
        return;
      }
      if (connect(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
                  (struct sockaddr*)&sock->remote.ipv4,
                  sizeof(sock->remote.ipv4)) < 0) {
        if (errno == EINPROGRESS) {
          Xen_IO_Status_SOut((Xen_IO_Status*)sock->f->ptr, coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
          *connecting = 1;
          return;
        }
        if (errno == EINTR) {
          return;
        }
        Xen_CallError(coro);
        Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
        return;
      }
      break;
    }
    case AF_INET6: {
      if (Xen_IsTuple(addr)) {
        struct Socket_Address_IP address;
        if (!Socket_Addr_IP_Get(addr, &address)) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
        memset(&sock->remote.ipv6, 0, sizeof(sock->remote.ipv6));
        sock->remote.ipv6.sin6_family = sock->domain;
        sock->remote.ipv6.sin6_port = htons(address.port);
        if (inet_pton(sock->domain, address.ip, &sock->remote.ipv6.sin6_addr) !=
            1) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
      } else if (Xen_IsBytes(addr)) {
        if (Xen_SIZE(addr) != sizeof(sock->remote.ipv6)) {
          Xen_CallError(coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
          return;
        }
        memcpy(&sock->remote.ipv6, Xen_Bytes_Get(addr),
               sizeof(sock->remote.ipv6));
      } else {
        Xen_CallError(coro);
        Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
        return;
      }
      if (connect(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
                  (struct sockaddr*)&sock->remote.ipv6,
                  sizeof(sock->remote.ipv6)) < 0) {
        if (errno == EINPROGRESS) {
          Xen_IO_Status_SOut((Xen_IO_Status*)sock->f->ptr, coro);
          Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
          *connecting = 1;
          return;
        }
        if (errno == EINTR) {
          return;
        }
        Xen_CallError(coro);
        Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
        return;
      }
      break;
    }
    default:
      Xen_CallError(coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
      return;
    }
    sock->caps |= SOCKET_CAP_READ | SOCKET_CAP_WRITE | SOCKET_CAP_CONNECT;
    Xen_COROUTINE_RETURN(nil);
  } else {
    int err = 0;
    socklen_t len = sizeof(int);
    if (getsockopt(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
               SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
      Xen_CallError(coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
      return;
    }
    if (err != 0) {
      Xen_CallError(coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
      return;
    }
    sock->caps |= SOCKET_CAP_READ | SOCKET_CAP_WRITE | SOCKET_CAP_CONNECT;
    Xen_COROUTINE_RETURN(nil);
  }
}

struct __asocket_send_status {
  const Xen_uint8_t* buffer;
  Xen_size_t offset;
  Xen_size_t total;
  int initialize;
};

static void asocket_send(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  ASocket* sock = (ASocket*)self;
  if (!sock->open) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  if (!(sock->caps & SOCKET_CAP_WRITE)) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  struct __asocket_send_status* status = Xen_Coroutine_Data(coro);
  if (!status->initialize) {
    Xen_Function_ArgSpec args_def[] = {
        {"data", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES,
         XEN_FUNCTION_ARG_REQUIRED, NULL},
        {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
    };

    Xen_Function_ArgBinding* binding =
        Xen_Function_ArgsParse(args, kwargs, args_def);
    if (!binding) {
      Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
      return;
    }
    Xen_Instance* data = Xen_Function_ArgBinding_Search(binding, "data")->value;
    Xen_Function_ArgBinding_Free(binding);
    status->buffer = Xen_Bytes_Get(data);
    status->offset = 0;
    status->total = Xen_SIZE(data);
    status->initialize = 1;
  }
  Xen_ssize_t s = send(*(int*)Xen_IO_Status_FD((Xen_IO_Status*)sock->f->ptr),
                       status->buffer + status->offset,
                       status->total - status->offset, 0);
  if (s < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      Xen_IO_Status_SOut((Xen_IO_Status*)sock->f->ptr, coro);
      Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
      return;
    }
    if (errno == EINTR) {
      return;
    }
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  if (s == 0) {
    Xen_CallError(coro);
    Xen_Coroutine_SStatus(coro, Xen_CORO_EXCEPTED);
    return;
  }
  status->offset += s;
  if (status->offset >= status->total) {
    Xen_COROUTINE_RETURN(Xen_Number_From_Long(status->offset));
  }
}

Xen_ImplementStruct ASocket_Implement = {
  .__impl_name = "ASocket",
  .__inst_size = sizeof(ASocket),
  .__inst_trace = asocket_trace,
  .__alloc = asocket_alloc,
  .__create = asocket_create,
  .__destroy = asocket_destroy,
  .__get_attr = Xen_Basic_Get_Attr_Static,
};

void ASocket_Init(Xen_Instance *module) {
  ASocket_Implement_Pointer = (Xen_Implement*)Xen_Attr_Get_Str(module, "ASocket");
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "bind", asocket_bind, nil);
  Xen_VM_Store_Native_Function(props, "listen", asocket_listen, nil);
  Xen_VM_Store_Native_Function(props, "shutdown", asocket_shutdown, nil);
  Xen_VM_Store_Native_Function(props, "setsockopt", asocket_setsockopt, nil);
  Xen_VM_Store_Native_Function(props, "getsockopt", asocket_getsockopt, nil);
  Xen_VM_Store_Native_Function(props, "set_nonblock", asocket_set_nonblocking, nil);
  Xen_VM_Store_Native_Function(props, "getsockname", asocket_getsockname, nil);
  Xen_VM_Store_Native_Function(props, "getpeername", asocket_getpeername, nil);
  Xen_VM_Store_Native_Function(props, "close", asocket_close, nil);
  Xen_VM_Store_Native_Function_Async(props, "accept", asocket_accept, 0);
  Xen_VM_Store_Native_Function_Async(props, "connect", asocket_connect, sizeof(int));
  Xen_VM_Store_Native_Function_Async(props, "send", asocket_send, sizeof(struct __asocket_send_status));
  Xen_Implement_SetProps(ASocket_Implement_Pointer, props);
}
