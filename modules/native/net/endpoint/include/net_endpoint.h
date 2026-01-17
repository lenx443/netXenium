#ifndef __NET_ENDPOINT_H__
#define __NET_ENDPOINT_H__

#include "netxenium/netXenium.h"

typedef struct {
  Xen_INSTANCE_HEAD;
  Xen_GCHandle* ip;
  Xen_uint16_t port;
} EndPoint;

extern Xen_Implement* ENDPOINT_IMPLEMENT_ptr;
extern Xen_ImplementStruct EndPoint_implmenet;

void EndPoint_init(Xen_Instance*);

#endif
