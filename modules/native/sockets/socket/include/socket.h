#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "netxenium/netXenium.h"

#include <arpa/inet.h>
#include <linux/if_packet.h>

#define SOCKET_CAP_READ (1 << 0)
#define SOCKET_CAP_WRITE (1 << 1)
#define SOCKET_CAP_BIND (1 << 2)
#define SOCKET_CAP_LISTEN (1 << 3)
#define SOCKET_CAP_ACCEPT (1 << 4)
#define SOCKET_CAP_CONNECT (1 << 5)
#define SOCKET_CAP_NONBLOCK (1 << 6)
#define SOCKET_CAP_TIMEOUT (1 << 7)

struct Socket_Address_Packet {
  Xen_c_string_t device;
  Xen_uint16_t protocol;
  Xen_uint8_t addr[6];
};

typedef struct {
  Xen_INSTANCE_HEAD int f;
  Xen_bool_t open;
  int domain;
  int type;
  int protocol;
  union {
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
    struct sockaddr_ll addr;
  } local;
  union {
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
    struct sockaddr_ll addr;
  } remote;
  Xen_uint32_t caps;
} Socket;

extern Xen_Implement *Socket_Implement_Pointer;
extern Xen_ImplementStruct Socket_Implement;

void Socket_init(Xen_Instance *);

#endif
