#include "netxenium/netXenium.h"

#include <sys/endian.h>

static Xen_Instance *fn_htons(Xen_Instance *self, Xen_Instance *args,
                              Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"short", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *s = Xen_Function_ArgBinding_Search(binding, "short")->value;
  Xen_Function_ArgBinding_Free(binding);
  int16_t r = htons(Xen_Number_As_Int(s));
  return Xen_Number_From_Int((uint16_t)r);
}

static Xen_Instance *fn_ntohs(Xen_Instance *self, Xen_Instance *args,
                              Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"short", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *s = Xen_Function_ArgBinding_Search(binding, "short")->value;
  Xen_Function_ArgBinding_Free(binding);
  int16_t r = ntohs(Xen_Number_As_Int(s));
  return Xen_Number_From_Int((uint16_t)r);
}

static Xen_Instance *fn_htonl(Xen_Instance *self, Xen_Instance *args,
                              Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"long", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *l = Xen_Function_ArgBinding_Search(binding, "short")->value;
  Xen_Function_ArgBinding_Free(binding);
  long r = htonl(Xen_Number_As_Long(l));
  return Xen_Number_From_Int((uint16_t)r);
}

static Xen_Instance *fn_ntohl(Xen_Instance *self, Xen_Instance *args,
                              Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"long", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *l = Xen_Function_ArgBinding_Search(binding, "short")->value;
  Xen_Function_ArgBinding_Free(binding);
  long r = ntohl(Xen_Number_As_Long(l));
  return Xen_Number_From_Long(r);
}

static struct Xen_Module_Function functions[] = {
    {"htons", fn_htons}, {"ntohs", fn_ntohs}, {"htonl", fn_htonl},
    {"ntohl", fn_ntohl}, {NULL, NULL},
};

struct Xen_Module_Def *Xen_Module_endian_Start(void *);
struct Xen_Module_Def *Xen_Module_endian_Start(void *globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("endian", NULL, functions, NULL, NULL);
}
