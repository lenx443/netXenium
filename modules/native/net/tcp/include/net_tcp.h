#ifndef __NET_TCP_H__
#define __NET_TCP_H__

#include "netxenium/netXenium.h"

Xen_Instance* Net_TCP_Server(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Net_TCP_Client(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Net_TCP_AServer(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Net_TCP_AClient(Xen_Instance*, Xen_Instance*, Xen_Instance*);

#endif
