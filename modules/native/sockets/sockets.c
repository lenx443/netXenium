#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "netxenium/implement.h"
#include "netxenium/netXenium.h"

#define SOCKET_CAP_READ (1 << 0)
#define SOCKET_CAP_WRITE (1 << 1)
#define SOCKET_CAP_BIND (1 << 2)
#define SOCKET_CAP_LISTEN (1 << 3)
#define SOCKET_CAP_ACCEPT (1 << 4)
#define SOCKET_CAP_CONNECT (1 << 5)
#define SOCKET_CAP_NONBLOCK (1 << 6)

static Xen_Implement* Socket_Implement_Pointer = NULL;

static void SocketTimeout(void) {
  Xen_VM_Except_Throw(Xen_Except_New("Timeout", "Operation timed out"));
}

struct Socket_Address_IP {
  Xen_c_string_t ip;
  int port;
};

static int Socket_Addr_IP_Get(Xen_Instance* addr,
                              struct Socket_Address_IP* out) {
  if (Xen_SIZE(addr) != 2) {
    return 0;
  }
  Xen_Instance* ip_inst = Xen_Vector_Get_Index(addr, 0);
  Xen_Instance* port_inst = Xen_Vector_Get_Index(addr, 1);
  if (!Xen_IsString(ip_inst) || !Xen_IsNumber(port_inst)) {
    return 0;
  }
  out->ip = Xen_String_As_CString(ip_inst);
  out->port = Xen_Number_As_Int(port_inst);
  return 1;
}

static Xen_Instance* Socket_Addr_IP_Set(struct Socket_Address_IP in) {
  if (!in.ip) {
    return NULL;
  }
  Xen_Instance* ip = Xen_String_From_CString(in.ip);
  Xen_Instance* port = Xen_Number_From_Int(ntohs(in.port));
  Xen_Instance* addr = Xen_Tuple_From_Array(2, (Xen_Instance*[]){ip, port});
  return addr;
}

typedef struct {
  Xen_INSTANCE_HEAD;
  int f;
  Xen_bool_t open;
  int domain;
  int type;
  int protocol;
  union {
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
  } local;
  union {
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
  } remote;
  Xen_uint32_t caps;
} Socket;

static Xen_Instance* socket_create(Xen_Instance* self, Xen_Instance* args,
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
    if (bind(sock->f, (struct sockaddr*)&sock->local.ipv4,
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
    if (bind(sock->f, (struct sockaddr*)&sock->local.ipv6,
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
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  if (!(sock->caps & SOCKET_CAP_ACCEPT)) {
    return NULL;
  }
  switch (sock->domain) {
  case AF_INET: {
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
        (Socket*)__instance_new(Socket_Implement_Pointer, nil, nil, 0);
    client->f = client_fd;
    client->open = 1;
    client->domain = sock->domain;
    client->type = sock->type;
    client->protocol = sock->protocol;
    client->remote.ipv4 = remote;
    client->local.ipv4 = sock->local.ipv4;
    client->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
    return (Xen_Instance*)client;
  }
  case AF_INET6: {
    struct sockaddr_in6 remote;
    socklen_t len = sizeof(remote);
    int client_fd = accept(sock->f, (struct sockaddr*)&remote, &len);
    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      return NULL;
    }
    Socket* client =
        (Socket*)__instance_new(Socket_Implement_Pointer, nil, nil, 0);
    client->f = client_fd;
    client->open = 1;
    client->domain = sock->domain;
    client->type = sock->type;
    client->protocol = sock->protocol;
    client->remote.ipv6 = remote;
    client->local.ipv6 = sock->local.ipv6;
    client->caps = SOCKET_CAP_READ | SOCKET_CAP_WRITE;
    return (Xen_Instance*)client;
  }
  default:
    return NULL;
  }
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
      {"addr", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_TUPLE,
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
      memset(&sock->remote.ipv4, 0, sizeof(sock->remote.ipv4));
      sock->remote.ipv4.sin_family = sock->domain;
      sock->remote.ipv4.sin_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &sock->remote.ipv4.sin_addr) !=
          1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(sock->remote.ipv4)) {
        return NULL;
      }
      memcpy(&sock->remote.ipv4, Xen_Bytes_Get(addr),
             sizeof(sock->remote.ipv4));
    } else {
      return NULL;
    }
    if (connect(sock->f, (struct sockaddr*)&sock->remote.ipv4,
                sizeof(sock->remote.ipv4)) < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
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
      memset(&sock->remote.ipv6, 0, sizeof(sock->remote.ipv6));
      sock->remote.ipv6.sin6_family = sock->domain;
      sock->remote.ipv6.sin6_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &sock->remote.ipv6.sin6_addr) !=
          1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(sock->remote.ipv6)) {
        return NULL;
      }
      memcpy(&sock->remote.ipv6, Xen_Bytes_Get(addr),
             sizeof(sock->remote.ipv6));
    } else {
      return NULL;
    }
    if (connect(sock->f, (struct sockaddr*)&sock->remote.ipv6,
                sizeof(sock->remote.ipv6)) < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      return NULL;
    }
    break;
  }
  default:
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

static Xen_Instance* socket_sendto(Xen_Instance* self, Xen_Instance* args,
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
      {"addr", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_TUPLE,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {Xen_NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, Xen_NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* data = Xen_Function_ArgBinding_Search(binding, "data")->value;
  Xen_Instance* addr = Xen_Function_ArgBinding_Search(binding, "addr")->value;
  Xen_Function_ArgBinding_Free(binding);
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    if (Xen_IsTuple(addr)) {
      struct Socket_Address_IP address;
      if (!Socket_Addr_IP_Get(addr, &address)) {
        return NULL;
      }
      remote.sin_family = sock->domain;
      remote.sin_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &remote.sin_addr) != 1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(remote)) {
        return NULL;
      }
      memcpy(&remote, Xen_Bytes_Get(addr), sizeof(remote));
    } else {
      return NULL;
    }
    Xen_ssize_t s = sendto(sock->f, Xen_Bytes_Get(data), Xen_SIZE(data), 0,
                           (struct sockaddr*)&remote, sizeof(remote));
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      return NULL;
    }
    return Xen_Number_From_Long(s);
  }
  case AF_INET6: {
    struct sockaddr_in6 remote;
    memset(&remote, 0, sizeof(remote));
    if (Xen_IsTuple(addr)) {
      struct Socket_Address_IP address;
      if (!Socket_Addr_IP_Get(addr, &address)) {
        return NULL;
      }
      remote.sin6_family = sock->domain;
      remote.sin6_port = htons(address.port);
      if (inet_pton(sock->domain, address.ip, &remote.sin6_addr) != 1) {
        return NULL;
      }
    } else if (Xen_IsBytes(addr)) {
      if (Xen_SIZE(addr) != sizeof(remote)) {
        return NULL;
      }
      memcpy(&remote, Xen_Bytes_Get(addr), sizeof(remote));
    } else {
      return NULL;
    }
    Xen_ssize_t s = sendto(sock->f, Xen_Bytes_Get(data), Xen_SIZE(data), 0,
                           (struct sockaddr*)&remote, sizeof(remote));
    if (s < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      return NULL;
    }
    return Xen_Number_From_Long(s);
  }
  default:
    return NULL;
  }
}

static Xen_Instance* socket_recvfrom(Xen_Instance* self, Xen_Instance* args,
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
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    socklen_t remote_len = sizeof(remote);
    Xen_string_t buffer = Xen_Alloc(size);
    Xen_ssize_t r = recvfrom(sock->f, buffer, size, 0,
                             (struct sockaddr*)&remote, &remote_len);
    if (r < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* data = Xen_Bytes_New();
    char ip_str[INET_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &remote.sin_addr, ip_str, sizeof(ip_str))) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* addr =
        Socket_Addr_IP_Set((struct Socket_Address_IP){ip_str, remote.sin_port});
    Xen_Instance* result =
        Xen_Tuple_From_Array(2, (Xen_Instance*[]){data, addr});
    if (r == 0) {
      Xen_Dealloc(buffer);
      return result;
    }
    Xen_Bytes_Append_Array(data, r, (Xen_uint8_t*)buffer);
    Xen_Dealloc(buffer);
    return result;
  }
  case AF_INET6: {
    struct sockaddr_in6 remote;
    memset(&remote, 0, sizeof(remote));
    socklen_t remote_len = sizeof(remote);
    Xen_string_t buffer = Xen_Alloc(size);
    Xen_ssize_t r = recvfrom(sock->f, buffer, size, 0,
                             (struct sockaddr*)&remote, &remote_len);
    if (r < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SocketTimeout();
      }
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* data = Xen_Bytes_New();
    char ip_str[INET6_ADDRSTRLEN];
    if (!inet_ntop(sock->domain, &remote.sin6_addr, ip_str, sizeof(ip_str))) {
      Xen_Dealloc(buffer);
      return NULL;
    }
    Xen_Instance* ip = Xen_String_From_CString(ip_str);
    Xen_Instance* port = Xen_Number_From_Int(ntohs(remote.sin6_port));
    Xen_Instance* addr = Xen_Tuple_From_Array(2, (Xen_Instance*[]){ip, port});
    Xen_Instance* result =
        Xen_Tuple_From_Array(2, (Xen_Instance*[]){data, addr});
    if (r == 0) {
      Xen_Dealloc(buffer);
      return result;
    }
    Xen_Bytes_Append_Array(data, r, (Xen_uint8_t*)buffer);
    Xen_Dealloc(buffer);
    return result;
  }
  default:
    return NULL;
  }
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
    if (fcntl(sock->f, F_SETFL, flags | O_NONBLOCK) == -1) {
      return NULL;
    }
    sock->caps |= SOCKET_CAP_NONBLOCK;
  } else {
    int flags = fcntl(sock->f, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if (fcntl(sock->f, F_SETFL, flags) == -1) {
      return NULL;
    }
    sock->caps &= ~SOCKET_CAP_NONBLOCK;
  }
  return nil;
}

static Xen_Instance* socket_set_timeout(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  Xen_Function_ArgSpec args_def[] = {
      {"sec", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, Xen_NULL},
      {"usec", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, Xen_NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_size_t sec =
      Xen_Number_As_Long(Xen_Function_ArgBinding_Search(binding, "sec")->value);
  Xen_Function_ArgBound* usec_arg =
      Xen_Function_ArgBinding_Search(binding, "usec");
  Xen_size_t usec = 0;
  if (usec_arg->provided) {
    usec = Xen_Number_As_Long(usec_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  struct timeval timev = {
      .tv_sec = sec,
      .tv_usec = usec,
  };
  if (setsockopt(sock->f, SOL_SOCKET, SO_SNDTIMEO, &timev, sizeof(timev)) < 0) {
    return NULL;
  }
  if (setsockopt(sock->f, SOL_SOCKET, SO_RCVTIMEO, &timev, sizeof(timev)) < 0) {
    return NULL;
  }
  return nil;
}

static Xen_Instance* socket_getsockname(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getsockname(sock->f, (struct sockaddr*)&local, &len) < 0) {
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
    if (getsockname(sock->f, (struct sockaddr*)&local, &len) < 0) {
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

static Xen_Instance* socket_getpeername(Xen_Instance* self, Xen_Instance* args,
                                        Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (!sock->open) {
    return NULL;
  }
  switch (sock->domain) {
  case AF_INET: {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t len = sizeof(local);
    if (getpeername(sock->f, (struct sockaddr*)&local, &len) < 0) {
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
    if (getpeername(sock->f, (struct sockaddr*)&local, &len) < 0) {
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

static Xen_Instance* socket_close(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Socket* sock = (Socket*)self;
  if (sock->open) {
    close(sock->f);
    sock->open = 0;
  }
  return nil;
}

static Xen_ImplementStruct Socket_Implement = {
    .__impl_name = "Socket",
    .__inst_size = sizeof(Socket),
    .__create = socket_create,
    .__destroy = socket_destroy,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

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
      if (!inet_ntop(p->ai_family,
                     &((struct sockaddr_in6*)p->ai_addr)->sin6_addr, buffer,
                     sizeof(buffer))) {
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
    Xen_Instance* rtuple =
        Xen_Tuple_From_Array(6, (Xen_Instance*[]){rflags, rfamily, rsocktype,
                                                  rproto, raddr, rcanonname});
    Xen_Vector_Push(result, rtuple);
  }
  freeaddrinfo(res);
  return result;
}

static Xen_Instance* Sockets_Init(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if ((Socket_Implement_Pointer =
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
  Xen_VM_Store_Native_Function(props, "sendto", socket_sendto, nil);
  Xen_VM_Store_Native_Function(props, "recvfrom", socket_recvfrom, nil);
  Xen_VM_Store_Native_Function(props, "shutdown", socket_shutdown, nil);
  Xen_VM_Store_Native_Function(props, "setsockopt", socket_setsockopt, nil);
  Xen_VM_Store_Native_Function(props, "getsockopt", socket_getsockopt, nil);
  Xen_VM_Store_Native_Function(props, "set_nonblocking", socket_set_nonblocking,
                               nil);
  Xen_VM_Store_Native_Function(props, "set_timeout", socket_set_timeout, nil);
  Xen_VM_Store_Native_Function(props, "getsockname", socket_getsockname, nil);
  Xen_VM_Store_Native_Function(props, "getpeername", socket_getpeername, nil);
  Xen_VM_Store_Native_Function(props, "close", socket_close, nil);
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"AF_INET", Xen_Number_From_Int(AF_INET)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"AF_INET6", Xen_Number_From_Int(AF_INET6)});
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"SOCK_STREAM", Xen_Number_From_Int(SOCK_STREAM)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SOCK_DGRAM", Xen_Number_From_Int(SOCK_DGRAM)});
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"IPPROTO_TCP", Xen_Number_From_Int(IPPROTO_TCP)});
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"IPPROTO_UDP", Xen_Number_From_Int(IPPROTO_UDP)});
  Xen_Map_Push_Pair_Str(
      props,
      (Xen_Map_Pair_Str){"IPPROTO_ICMP", Xen_Number_From_Int(IPPROTO_ICMP)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"IPPROTO_ICMPV6",
                                Xen_Number_From_Int(IPPROTO_ICMPV6)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_RD", Xen_Number_From_Int(SHUT_RD)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_WR", Xen_Number_From_Int(SHUT_WR)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"SHUT_RDWR", Xen_Number_From_Int(SHUT_RDWR)});
  Xen_Implement_SetProps(Socket_Implement_Pointer, props);
  return nil;
}

struct Xen_Module_Function functions[] = {
    {"getaddrinfo", sockets_getaddrinfo},
    {NULL, NULL},
};

static Xen_ImplementStruct* implements[] = {
    &Socket_Implement,
    NULL,
};

struct Xen_Module_Def* Xen_Module_sockets_Start(void*);
struct Xen_Module_Def* Xen_Module_sockets_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("sockets", Sockets_Init, functions, implements);
}
