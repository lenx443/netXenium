#ifndef __NET_IP_H__
#define __NET_IP_H__

#include "netxenium/netXenium.h"

#include <bits/in_addr.h>
#include <linux/in6.h>

#define NET_IPV4 1
#define NET_IPV6 2

typedef struct {
  Xen_INSTANCE_HEAD;
  Xen_uint8_t ip_type;
  union {
    struct in_addr ipv4;
    struct in6_addr ipv6;
  };
} IP;

extern Xen_Implement* IP_IMPLEMENT_ptr;
extern Xen_ImplementStruct IP_implmenet;

void IP_init(Xen_Instance*);

#endif
