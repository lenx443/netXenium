#include "netxenium/netXenium.h"

static Xen_Instance* checksum(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
    {"data", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES, XEN_FUNCTION_ARG_REQUIRED, NULL},
    {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance* data = Xen_Function_ArgBinding_Search(binding, "data")->value;
  Xen_Function_ArgBinding_Free(binding);
  const uint8_t *bytes = Xen_Bytes_Get(data);
  Xen_size_t len = Xen_SIZE(data);
  uint32_t sum = 0;
  while (len > 1) {
      sum += ((uint16_t)bytes[0] << 8) | bytes[1];
      bytes += 2;
      len -= 2;
  }
  if (len)
      sum += (uint16_t)bytes[0] << 8;
  while (sum >> 16)
      sum = (sum & 0xffff) + (sum >> 16);
  return Xen_Number_From_UInt((uint16_t)~sum);
}

static struct Xen_Module_Function functions[] = {
  {"checksum", checksum},
  {NULL, NULL},
};

struct Xen_Module_Def* Xen_Module_checksum_Start(void*);
struct Xen_Module_Def* Xen_Module_checksum_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("checksum", NULL, functions, NULL, NULL);
}
