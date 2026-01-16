#include "net_ip.h"
#include "netxenium/netXenium.h"

#include <arpa/inet.h>
#include <netinet/in.h>

static Xen_Instance* ip_create(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  IP* ip = (IP*)self;
  Xen_Function_ArgSpec args_def[] = {
      {"ip", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_c_string_t ip_str = Xen_String_As_CString(
      Xen_Function_ArgBinding_Search(binding, "ip")->value);
  Xen_Function_ArgBinding_Free(binding);
  for (Xen_c_string_t i = ip_str; *i != '\0'; i++) {
    if (*i == '.') {
      ip->ip_type = NET_IPV4;
      if (inet_pton(AF_INET, ip_str, &ip->ipv4) != 1) {
        return NULL;
      }
      return nil;
      break;
    }
    if (*i == ':') {
      ip->ip_type = NET_IPV6;
      if (inet_pton(AF_INET6, ip_str, &ip->ipv6) != 1) {
        return NULL;
      }
      return nil;
      break;
    }
  }
  return NULL;
}

static Xen_Instance* ip_prop_string(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  IP* ip = (IP*)self;
  if (ip->ip_type == NET_IPV4) {
    char buffer[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &ip->ipv4, buffer, sizeof(buffer))) {
      return NULL;
    }
    return Xen_String_From_CString(buffer);
  } else if (ip->ip_type == NET_IPV6) {
    char buffer[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, &ip->ipv6, buffer, sizeof(buffer))) {
      return NULL;
    }
    return Xen_String_From_CString(buffer);
  }
  return NULL;
}

static Xen_Instance* ip_type(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  IP* ip = (IP*)self;
  return Xen_Number_From_Int(ip->ip_type);
}

Xen_Implement* IP_IMPLEMENT_ptr = NULL;
Xen_ImplementStruct IP_implmenet = {
    .__impl_name = "IP",
    .__inst_size = sizeof(IP),
    .__create = ip_create,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

void IP_init(Xen_Instance* module) {
  IP_IMPLEMENT_ptr = (Xen_Implement*)Xen_Attr_Get_Str(module, "IP");
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "string", ip_prop_string, nil);
  Xen_VM_Store_Native_Function(props, "type", ip_type, nil);
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"IPV4", Xen_Number_From_Int(NET_IPV4)});
  Xen_Map_Push_Pair_Str(
      props, (Xen_Map_Pair_Str){"IPV6", Xen_Number_From_Int(NET_IPV6)});
  Xen_Implement_SetProps(IP_IMPLEMENT_ptr, props);
}
