#include "net_endpoint.h"
#include "net_ip.h"
#include "netxenium/netXenium.h"
#include <string.h>

static Xen_bool_t parse_u16(Xen_c_string_t s, Xen_uint16_t* out) {
  uint32_t acc = 0;

  if (!s || !*s)
    return 0;

  while (*s) {
    char c = *s++;

    if (c < '0' || c > '9')
      return 0;

    uint32_t d = (uint32_t)(c - '0');

    if (acc > (UINT16_MAX - d) / 10)
      return 0;

    acc = acc * 10 + d;
  }

  *out = (uint16_t)acc;
  return 1;
}

static void endpoint_trace(Xen_Instance* inst) {
  EndPoint* endpoint = (EndPoint*)inst;
  if (endpoint->ip->ptr) {
    Xen_GC_Trace_GCHeader(endpoint->ip);
  }
}

static Xen_Instance* endpoint_alloc(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  EndPoint* endpoint = (EndPoint*)Xen_Instance_Alloc(ENDPOINT_IMPLEMENT_ptr);
  endpoint->ip = Xen_GCHandle_New((Xen_GCHeader*)endpoint);
  return (Xen_Instance*)endpoint;
}

static Xen_Instance* endpoint_create(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  EndPoint* endpoint = (EndPoint*)self;
  Xen_Function_ArgSpec args_def[] = {
      {"endp", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
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
  Xen_c_string_t endp = Xen_String_As_CString(
      Xen_Function_ArgBinding_Search(binding, "endp")->value);
  Xen_Function_ArgBound* type_arg =
      Xen_Function_ArgBinding_Search(binding, "type");
  Xen_Instance* type = NULL;
  if (type_arg->provided) {
    type = type_arg->value;
  } else {
    type = Xen_Number_From_Int(0);
  }
  Xen_Function_ArgBinding_Free(binding);
  Xen_c_string_t ip_start;
  Xen_size_t ip_len;
  Xen_c_string_t port_start;
  Xen_size_t port_len;
  Xen_c_string_t p = endp;
  if (*p == '\0') {
    return NULL;
  } else if (*p == ':') {
    p++;
    ip_start = endp;
    ip_len = 0;
    port_start = p;
    while (*p != '\0') {
      p++;
    }
    port_len = p - port_start;
  } else if (*p == '[') {
    p++;
    ip_start = p;
    while (*p != ']') {
      if (*p == '\0') {
        return NULL;
      }
      p++;
    }
    ip_len = p - ip_start;
    p++;
    if (*p++ != ':') {
      return NULL;
    }
    port_start = p;
    while (*p != '\0') {
      p++;
    }
    port_len = p - port_start;
  } else {
    ip_start = p;
    while (*p != ':') {
      if (*p == '\0') {
        return NULL;
      }
      p++;
    }
    ip_len = p - ip_start;
    port_start = ++p;
    while (*p != '\0') {
      p++;
    }
    port_len = p - port_start;
  }
  Xen_string_t ip_str = Xen_Alloc(ip_len + 1);
  Xen_string_t port_str = Xen_Alloc(port_len + 1);
  strncpy(ip_str, ip_start, ip_len);
  strncpy(port_str, port_start, port_len);
  ip_str[ip_len] = '\0';
  port_str[port_len] = '\0';
  int type_value = Xen_Number_As_Int(type);
  Xen_Instance* ip_inst = Xen_String_From_CString(
      ip_len > 0 ? ip_str : (type_value == NET_IPV6 ? "::" : "0.0.0.0"));
  Xen_Instance* ip_args =
      Xen_Tuple_From_Array(2, (Xen_Instance*[]){ip_inst, type});
  Xen_Instance* ip = Xen_Create(IP_IMPLEMENT_ptr, ip_args, nil);
  if (!ip) {
    Xen_Dealloc(ip_str);
    Xen_Dealloc(port_str);
    return NULL;
  }
  Xen_IGC_WRITE_FIELD(endpoint, endpoint->ip, ip);
  if (!parse_u16(port_str, &endpoint->port)) {
    Xen_Dealloc(ip_str);
    Xen_Dealloc(port_str);
    return NULL;
  }
  endpoint->port = htons(endpoint->port);
  Xen_Dealloc(ip_str);
  Xen_Dealloc(port_str);
  return nil;
}

static Xen_Instance* endpoint_destroy(Xen_Instance* self, Xen_Instance* args,
                                      Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  EndPoint* endpoint = (EndPoint*)self;
  Xen_GCHandle_Free(endpoint->ip);
  return nil;
}

static Xen_Instance* endpoint_ip(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  EndPoint* endpoint = (EndPoint*)self;
  return (Xen_Instance*)endpoint->ip->ptr;
}

static Xen_Instance* endpoint_port(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  EndPoint* endpoint = (EndPoint*)self;
  return Xen_Number_From_Int(endpoint->port);
}

static Xen_Instance* endpoint_tuple(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  EndPoint* endpoint = (EndPoint*)self;
  Xen_string_t ip_str = Net_IP_As_CString((Xen_Instance*)endpoint->ip->ptr);
  if (!ip_str) {
    return NULL;
  }
  Xen_Instance* ip = Xen_String_From_CString(ip_str);
  Xen_Dealloc(ip_str);
  Xen_Instance* port = Xen_Number_From_Int(endpoint->port);
  Xen_Instance* tuple = Xen_Tuple_From_Array(2, (Xen_Instance*[]){ip, port});
  return tuple;
}
static Xen_Instance* endpoint_bytes(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Xen_Instance* bytes = NULL;
  IP* ip = (IP*)Net_EndPoint_IP(self);
  Xen_uint16_t port = Net_EndPoint_Port(self);
  if (ip->ip_type == NET_IPV4) {
    struct sockaddr_in sin;
    memcpy(&sin.sin_addr, &ip->ipv4, sizeof(sin.sin_addr));
    sin.sin_port = port;
    sin.sin_family = AF_INET;
    bytes = Xen_Bytes_From_Array(sizeof(sin), (Xen_uint8_t*)&sin);
  } else if (ip->ip_type == NET_IPV6) {
    struct sockaddr_in6 sin6;
    memcpy(&sin6.sin6_addr, &ip->ipv6, sizeof(sin6.sin6_addr));
    sin6.sin6_port = port;
    sin6.sin6_family = AF_INET6;
    bytes = Xen_Bytes_From_Array(sizeof(sin6), (Xen_uint8_t*)&sin6);
  } else {
    return NULL;
  }
  return bytes;
}

Xen_Implement* ENDPOINT_IMPLEMENT_ptr = NULL;
Xen_ImplementStruct EndPoint_implmenet = {
    .__impl_name = "Endpoint",
    .__inst_size = sizeof(EndPoint),
    .__inst_trace = endpoint_trace,
    .__alloc = endpoint_alloc,
    .__create = endpoint_create,
    .__destroy = endpoint_destroy,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

void EndPoint_init(Xen_Instance* module) {
  ENDPOINT_IMPLEMENT_ptr = (Xen_Implement*)Xen_Attr_Get_Str(module, "Endpoint");
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "ip", endpoint_ip, nil);
  Xen_VM_Store_Native_Function(props, "port", endpoint_port, nil);
  Xen_VM_Store_Native_Function(props, "tuple", endpoint_tuple, nil);
  Xen_VM_Store_Native_Function(props, "bytes", endpoint_bytes, nil);
  Xen_Implement_SetProps(ENDPOINT_IMPLEMENT_ptr, props);
}

Xen_Instance* Net_EndPoint_IP(Xen_Instance* endpoint) {
  return (Xen_Instance*)((EndPoint*)endpoint)->ip->ptr;
}

Xen_uint16_t Net_EndPoint_Port(Xen_Instance* endpoint) {
  return ((EndPoint*)endpoint)->port;
}
