#ifndef __AINET6_H__
#define __AINET6_H__

#include "netxenium/netXenium.h"
#include <arpa/inet.h>
#include <linux/in6.h>

typedef struct {
  Xen_INSTANCE_HEAD struct in6_addr addr;
  Xen_uint16_t port;
} AInet6;

extern Xen_Implement *AInet6_Implement_Pointer;
extern Xen_ImplementStruct AInet6_Implement;

void AInet6_Init(Xen_Instance *);

Xen_Instance *AInet6_From_Struct(struct sockaddr_in6 *);
void AInet6_Get(Xen_Instance *, struct sockaddr_in6 *);

#endif
