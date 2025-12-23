#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/fcntl.h>
#include <errno.h>
#include <linux/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "netxenium/netXenium.h"

#define SOCKET_AF_INET 1

#define SOCKET_SOCK_STREAM 1

#define SOCKET_CAP_READ (1 << 0)
#define SOCKET_CAP_WRITE (1 << 1)
#define SOCKET_CAP_BIND (1 << 2)
#define SOCKET_CAP_LISTEN (1 << 3)
#define SOCKET_CAP_ACCEPT (1 << 4)
#define SOCKET_CAP_CONNECT (1 << 5)
#define SOCKET_CAP_NONBLOCK (1 << 6)

static Xen_Implement* Socket_Implememnt_Pointer = NULL;

static void SocketTimeout(void) {
  Xen_VM_Except_Throw(Xen_Except_New("Timeout", "Operation timed out"));
}

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
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
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

static Xen_Instance* socket_listen(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
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
  if (listen(sock->f, backlog) < 0) {
    return NULL;
  }
  sock->caps |= SOCKET_CAP_LISTEN | SOCKET_CAP_ACCEPT;
  sock->caps &= ~(SOCKET_CAP_READ | SOCKET_CAP_WRITE);
  return nil;
}

static Xen_Instance* socket_accept(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpy(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (!(sock->caps & SOCKET_CAP_ACCEPT)) {
    return NULL;
  }
  struct sockaddr_in remote;
  socklen_t len = sizeof(remote);
  int client_fd = accept(sock->f, (struct sockaddr*)&remote, &len);
  if (client_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SocketTimeout();
    }
    return NULL;
  }
  Socket* client =
      (Socket*)__instance_new(Socket_Implememnt_Pointer, nil, nil, 0);
  client->f = client_fd;
  client->open = 1;
  client->domain = sock->domain;
  client->type = sock->type;
  client->protocol = sock->protocol;
  client->remote = remote;
  client->local = sock->local;
  client->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
  return (Xen_Instance*)client;
}

static Xen_Instance* socket_connect(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (sock->type != SOCK_STREAM) {
    return NULL;
  }
  if (sock->caps & (SOCKET_CAP_LISTEN | SOCKET_CAP_ACCEPT)) {
    return NULL;
  }
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
  memset(&sock->remote, 0, sizeof(sock->remote));
  sock->remote.sin_family = sock->domain;
  sock->remote.sin_port = htons(port);
  if (inet_pton(sock->domain, ip, &sock->remote.sin_addr) != 1) {
    return NULL;
  }
  if (connect(sock->f, (struct sockaddr*)&sock->remote, sizeof(sock->remote)) <
      0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SocketTimeout();
    }
    return NULL;
  }
  sock->caps |= SOCKET_CAP_READ | SOCKET_CAP_WRITE | SOCKET_CAP_CONNECT;
  return nil;
}

static Xen_Instance* socket_send(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (!(sock->caps & SOCKET_CAP_WRITE)) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"data", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* data = Xen_Function_ArgBinding_Search(binding, "data")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_ssize_t s = send(sock->f, Xen_Bytes_Get(data), Xen_SIZE(data), 0);
  if (s < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SocketTimeout();
    }
    return NULL;
  }
  return Xen_Number_From_Long(s);
}

static Xen_Instance* socket_recv(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (!(sock->caps & SOCKET_CAP_READ)) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"size", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  int size = 4096;
  Xen_Function_ArgBound* size_arg =
      Xen_Function_ArgBinding_Search(binding, "size");
  if (size_arg->provided) {
    size = Xen_Number_As_Int(size_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  Xen_string_t buffer = Xen_Alloc(size);
  Xen_ssize_t r = recv(sock->f, buffer, size, 0);
  if (r < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      SocketTimeout();
    }
    Xen_Dealloc(buffer);
    return NULL;
  }
  if (r == 0) {
    Xen_Dealloc(buffer);
    return Xen_Bytes_New();
  }
  Xen_Instance* data = Xen_Bytes_From_Array(r, (Xen_uint8_t*)buffer);
  Xen_Dealloc(buffer);
  return data;
}

static Xen_Instance* socket_shutdown(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
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
  if (shutdown(sock->f, how) < 0) {
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

static Xen_Instance* socket_setsockopt(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
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
  if (setsockopt(sock->f, level, optname, Xen_Bytes_Get(value),
                 Xen_SIZE(value)) < 0) {
    return NULL;
  }
  return nil;
}

static Xen_Instance* socket_getsockopt(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
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
  if (getsockopt(sock->f, level, optname, buffer, &buflen) < 0) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Instance* bytes = Xen_Bytes_From_Array(buflen, buffer);
  Xen_Dealloc(buffer);
  return bytes;
}

static Xen_Instance* socket_set_nonblocking(Xen_Instance* self,
                                            Xen_Instance* args,
                                            Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
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
    int flags = fcntl(sock->f, F_GETFL, 0);
    fcntl(sock->f, F_SETFL, flags | O_NONBLOCK);
    sock->caps |= SOCKET_CAP_NONBLOCK;
  } else {
    int flags = fcntl(sock->f, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(sock->f, F_SETFL, flags);
    sock->caps &= ~SOCKET_CAP_NONBLOCK;
  }
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
  Xen_VM_Store_Native_Function(props, "listen", socket_listen, nil);
  Xen_VM_Store_Native_Function(props, "accept", socket_accept, nil);
  Xen_VM_Store_Native_Function(props, "connect", socket_connect, nil);
  Xen_VM_Store_Native_Function(props, "send", socket_send, nil);
  Xen_VM_Store_Native_Function(props, "recv", socket_recv, nil);
  Xen_VM_Store_Native_Function(props, "shutdown", socket_shutdown, nil);
  Xen_VM_Store_Native_Function(props, "setsockopt", socket_setsockopt, nil);
  Xen_VM_Store_Native_Function(props, "getsockopt", socket_getsockopt, nil);
  Xen_VM_Store_Native_Function(props, "set_nonblocking", socket_set_nonblocking,
                               nil);
  Xen_VM_Store_Native_Function(props, "close", socket_close, nil);
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"AF_INET", Xen_Number_From_Int(SOCKET_AF_INET)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SOCK_STREAM",
                                Xen_Number_From_Int(SOCKET_SOCK_STREAM)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_RD", Xen_Number_From_Int(SHUT_RD)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_WR", Xen_Number_From_Int(SHUT_WR)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_RDWR", Xen_Number_From_Int(SHUT_RDWR)});
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
