#ifndef __ASOCKET_H__
#define __ASOCKET_H__

#include "netxenium/netXenium.h"

#include <arpa/inet.h>

typedef struct {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* f;
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
} ASocket;

extern Xen_Implement* ASocket_Implement_Pointer;
extern Xen_ImplementStruct ASocket_Implement;

void ASocket_Init(Xen_Instance*);

#endif
