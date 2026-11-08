#include "apacket.h"
#include "netxenium/netXenium.h"
#include <net/if.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/endian.h>

static int hex_to_int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool mac_to_bytes(const char *mac, Xen_uint8_t out[6]) {
  if (!mac || !out) return false;
  if (Xen_CString_Len(mac) != 17) return false;
  Xen_uint8_t result[6];
  for (Xen_size_t i = 0; i < 6; ++i) {
    Xen_size_t offset = i * 3;
    int high = hex_to_int(mac[offset]);
    int low = hex_to_int(mac[offset + 1]);
    if (high < 0 || low < 0) return false;
    if (i < 5 && mac[offset + 2] != ':') return false;
    result[i] = (Xen_uint8_t)((high << 4) | low);
  }
  memcpy(out, result, sizeof(result));
  return true;
}

Xen_Implement *APacket_Implement_Pointer = NULL;

static Xen_Instance *apacket_create(Xen_Instance *self, Xen_Instance *args,
                                    Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  Xen_Function_ArgSpec args_def[] = {
      {"iface", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"protocol", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"addr", XEN_FUNCTION_ARG_KIND_KEYWORD, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) { return NULL; }
  Xen_c_string_t iface =
      Xen_String_As_CString(Xen_Function_ArgBinding_Search(binding, "iface")->value);
  Xen_uint16_t protocol =
      Xen_Number_As_UInt(Xen_Function_ArgBinding_Search(binding, "protocol")->value);
  Xen_Instance *addr_inst = Xen_Function_ArgBinding_Search(binding, "addr")->value;
  Xen_Function_ArgBinding_Free(binding);
  apkt->ifindex = if_nametoindex(iface);
  apkt->protocol = htons(protocol);
  if (addr_inst) {
    Xen_c_string_t addr = Xen_String_As_CString(addr_inst);
    Xen_uint8_t result[6];
    if (!mac_to_bytes(addr, result)) { return NULL; }
    memcpy(apkt->addr, result, 6);
    apkt->halen = 6;
  }
  return nil;
}

static Xen_Instance *apacket_iface(Xen_Instance *self, Xen_Instance *args,
                                   Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  char ifa[IFNAMSIZ];
  if (if_indextoname(apkt->ifindex, ifa) == NULL) { return NULL; };
  ifa[IFNAMSIZ - 1] = '\0';
  return Xen_String_From_CString(ifa);
}

static Xen_Instance *apacket_ifindex(Xen_Instance *self, Xen_Instance *args,
                                     Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  return Xen_Number_From_Int(apkt->ifindex);
}

static Xen_Instance *apacket_protocol(Xen_Instance *self, Xen_Instance *args,
                                      Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  return Xen_Number_From_UInt(ntohs(apkt->protocol));
}

static Xen_Instance *apacket_addr(Xen_Instance *self, Xen_Instance *args,
                                  Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  char mac[18];
  snprintf(mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X", apkt->addr[0], apkt->addr[1],
           apkt->addr[2], apkt->addr[3], apkt->addr[4], apkt->addr[5]);
  mac[17] = '\0';
  return Xen_String_From_CString(mac);
}

static Xen_Instance *apacket_halen(Xen_Instance *self, Xen_Instance *args,
                                   Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  return Xen_Number_From_UInt(apkt->halen);
}

static Xen_Instance *apacket_hatype(Xen_Instance *self, Xen_Instance *args,
                                    Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  return Xen_Number_From_UInt(apkt->hatype);
}

static Xen_Instance *apacket_pkttype(Xen_Instance *self, Xen_Instance *args,
                                     Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  APacket *apkt = (APacket *)self;
  if (!Xen_Function_ArgEmpty(args, kwargs)) { return NULL; }
  return Xen_Number_From_UInt(apkt->pkttype);
}

Xen_ImplementStruct APacket_Implement = {
    .__impl_name = "APacket",
    .__inst_size = sizeof(APacket),
    .__create = apacket_create,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

void APacket_Init(Xen_Instance *module) {
  APacket_Implement_Pointer = (Xen_Implement *)Xen_Attr_Get_Str(module, "APacket");
  Xen_Instance *props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "iface", apacket_iface, nil);
  Xen_VM_Store_Native_Function(props, "ifindex", apacket_ifindex, nil);
  Xen_VM_Store_Native_Function(props, "protocol", apacket_protocol, nil);
  Xen_VM_Store_Native_Function(props, "addr", apacket_addr, nil);
  Xen_VM_Store_Native_Function(props, "halen", apacket_halen, nil);
  Xen_VM_Store_Native_Function(props, "hatype", apacket_hatype, nil);
  Xen_VM_Store_Native_Function(props, "pkttype", apacket_pkttype, nil);
  Xen_Implement_SetProps(APacket_Implement_Pointer, props);
}

Xen_Instance *APacket_From_Struct(struct sockaddr_ll *sll) {
  APacket *apkt = (APacket *)__instance_new(APacket_Implement_Pointer, nil, nil, 0);
  apkt->ifindex = sll->sll_ifindex;
  apkt->protocol = sll->sll_protocol;
  memcpy(apkt->addr, sll->sll_addr, 8);
  apkt->halen = sll->sll_halen;
  apkt->hatype = sll->sll_hatype;
  apkt->pkttype = sll->sll_pkttype;
  return (Xen_Instance *)apkt;
}

void APacket_Get(Xen_Instance *inst, struct sockaddr_ll *sll) {
  APacket *apkt = (APacket *)inst;
  sll->sll_family = AF_PACKET;
  sll->sll_ifindex = apkt->ifindex;
  sll->sll_protocol = apkt->protocol;
  memcpy(sll->sll_addr, apkt->addr, 8);
  sll->sll_halen = apkt->halen;
  sll->sll_hatype = sll->sll_hatype;
  sll->sll_pkttype = apkt->pkttype;
}
