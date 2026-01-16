#include "net_ip.h"
#include "netxenium/netXenium.h"

#include <arpa/inet.h>
#include <netinet/in.h>

extern Xen_Instance* MODULE_SOCKETS;

static Xen_Instance* GetAddrInfo(const char* node) {
  Xen_Instance* node_inst = Xen_String_From_CString(node);
  Xen_Instance* args = Xen_Tuple_From_Array(1, &node_inst);
  return Xen_Method_Attr_Str_Call(MODULE_SOCKETS, "getaddrinfo", args, nil);
}

static int is_domain(const char* s) {
  int label_len = 0;
  int has_alpha = 0;

  if (*s == '.' || *s == '-')
    return 0;

  for (; *s; s++) {
    if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) {
      has_alpha = 1;
      label_len++;
    } else if (*s >= '0' && *s <= '9') {
      label_len++;
    } else if (*s == '-') {
      if (label_len == 0)
        return 0;
      label_len++;
    } else if (*s == '.') {
      if (label_len == 0 || label_len > 63)
        return 0;
      label_len = 0;
    } else {
      return 0;
    }
  }

  return (label_len > 0 && label_len <= 63 && has_alpha);
}

static Xen_Instance* ip_create(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  IP* ip = (IP*)self;
  Xen_Function_ArgSpec args_def[] = {
      {"ip", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"type", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_c_string_t node = Xen_String_As_CString(
      Xen_Function_ArgBinding_Search(binding, "ip")->value);
  Xen_Function_ArgBound* type_arg =
      Xen_Function_ArgBinding_Search(binding, "type");
  int type = 0;
  if (type_arg->provided) {
    type = Xen_Number_As_Int(type_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  for (Xen_c_string_t i = node; *i != '\0'; i++) {
    if (*i == '.') {
      if (type != 0 && type != NET_IPV4) {
        break;
      }
      ip->ip_type = NET_IPV4;
      if (inet_pton(AF_INET, node, &ip->ipv4) != 1) {
        break;
      }
      return nil;
    }
    if (*i == ':') {
      if (type != 0 && type != NET_IPV6) {
        break;
      }
      ip->ip_type = NET_IPV6;
      if (inet_pton(AF_INET6, node, &ip->ipv6) != 1) {
        break;
      }
      return nil;
    }
  }
  if (is_domain(node)) {
    Xen_Instance* ai = GetAddrInfo(node);
    for (Xen_size_t i = 0; i < Xen_SIZE(ai); i++) {
      Xen_Instance* c = Xen_Vector_Get_Index(ai, i);
      int family = Xen_Number_As_Int(Xen_Tuple_Get_Index(c, 1));
      Xen_c_string_t ip_str = Xen_String_As_CString(Xen_Tuple_Get_Index(c, 4));
      if (type == 0) {
        if (family == AF_INET) {
          ip->ip_type = NET_IPV4;
          if (inet_pton(AF_INET, ip_str, &ip->ipv4) != 1) {
            break;
          }
          return nil;
        } else if (family == AF_INET6) {
          ip->ip_type = NET_IPV6;
          if (inet_pton(AF_INET6, ip_str, &ip->ipv6) != 1) {
            break;
          }
          return nil;
        }
      } else if (type == NET_IPV4 && family == AF_INET) {
        ip->ip_type = NET_IPV4;
        if (inet_pton(AF_INET, ip_str, &ip->ipv4) != 1) {
          break;
        }
        return nil;
      } else if (type == NET_IPV6 && family == AF_INET6) {
        ip->ip_type = NET_IPV6;
        if (inet_pton(AF_INET6, ip_str, &ip->ipv6) != 1) {
          break;
        }
        return nil;
      }
    }
  }
  return NULL;
}

static Xen_Instance* ip_prop_string(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
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
