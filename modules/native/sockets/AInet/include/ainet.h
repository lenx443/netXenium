#ifndef __AINET_H__
#define __AINET_H__

#include "netxenium/instance.h"
#include "netxenium/netXenium.h"
#include <arpa/inet.h>

typedef struct {
  Xen_INSTANCE_HEAD in_addr_t addr;
  Xen_uint16_t port;
} AInet;

extern Xen_Implement *AInet_Implement_Pointer;
extern Xen_ImplementStruct AInet_Implement;

void AInet_Init(Xen_Instance *);

Xen_Instance *AInet_From_Struct(struct sockaddr_in *);
void AInet_Get(Xen_Instance *, struct sockaddr_in *);

#endif
