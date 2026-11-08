#ifndef __APACKET_H__
#define __APACKET_H__

#include "netxenium/netXenium.h"
#include <linux/if_packet.h>

typedef struct {
  Xen_INSTANCE_HEAD int ifindex;
  Xen_uint16_t protocol;
  unsigned char addr[8];
  unsigned char halen;
  unsigned short hatype;
  unsigned char pkttype;
} APacket;

extern Xen_Implement *APacket_Implement_Pointer;
extern Xen_ImplementStruct APacket_Implement;

void APacket_Init(Xen_Instance *);
Xen_Instance *APacket_From_Struct(struct sockaddr_ll *);
void APacket_Get(Xen_Instance *, struct sockaddr_ll *);

#endif
