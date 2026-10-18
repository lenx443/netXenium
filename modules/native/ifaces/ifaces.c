#include "netxenium/netXenium.h"

#include <arpa/inet.h>
#include <bits/ioctl.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static Xen_Instance *get_addr(const char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return NULL;
  if (ioctl(sockfd, SIOCGIFADDR, &ifr)) {
    close(sockfd);
    return NULL;
  }
  char *str = inet_ntoa(addr->sin_addr);
  if (!str) {
    close(sockfd);
    return NULL;
  }
  close(sockfd);
  return Xen_String_From_CString(str);
}

static int set_addr(const char *iface, const char *value) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr));
  inet_aton(value, &addr.sin_addr);
  addr.sin_family = AF_INET;
  memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return 0;
  if (ioctl(sockfd, SIOCSIFADDR, &ifr)) {
    close(sockfd);
    return 0;
  }
  close(sockfd);
  return 1;
}

static Xen_Instance *get_netmask(const char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_netmask;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return NULL;
  if (ioctl(sockfd, SIOCGIFNETMASK, &ifr)) {
    close(sockfd);
    return NULL;
  }
  char *str = inet_ntoa(addr->sin_addr);
  if (!str) {
    close(sockfd);
    return NULL;
  }
  close(sockfd);
  return Xen_String_From_CString(str);
}

static int set_netmask(const char *iface, const char *value) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  inet_aton(value, &addr.sin_addr);
  addr.sin_family = AF_INET;
  memcpy(&ifr.ifr_netmask, &addr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return 0;
  if (ioctl(sockfd, SIOCSIFNETMASK, &ifr)) {
    close(sockfd);
    return 0;
  }
  close(sockfd);
  return 1;
}

static Xen_Instance *get_broadcast(const char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_broadaddr;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return NULL;
  if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr)) {
    close(sockfd);
    return NULL;
  }
  char *str = inet_ntoa(addr->sin_addr);
  if (!str) {
    close(sockfd);
    return NULL;
  }
  close(sockfd);
  return Xen_String_From_CString(str);
}

static int set_broadcast(const char *iface, const char *value) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  inet_aton(value, &addr.sin_addr);
  addr.sin_family = AF_INET;
  memcpy(&ifr.ifr_broadaddr, &addr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return 0;
  if (ioctl(sockfd, SIOCSIFBRDADDR, &ifr)) {
    close(sockfd);
    return 0;
  }
  close(sockfd);
  return 1;
}

static Xen_Instance *get_hwaddr(const char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return NULL;
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr)) {
    close(sockfd);
    return NULL;
  }
  char str[18];
  char hwaddr[6];
  memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, 6);
  snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X", hwaddr[0], hwaddr[1], hwaddr[2],
           hwaddr[3], hwaddr[4], hwaddr[5]);
  close(sockfd);
  return Xen_String_From_CString(str);
}

static int set_hwaddr(const char *iface, const char *value) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  ifr.ifr_hwaddr.sa_family = AF_INET;
  char str[18];
  char hwaddr[6];
  strncpy(str, value, sizeof(str) - 1);
  str[sizeof(str) - 1] = '\0';
  int n = 0;
  char *iterator = strtok(str, ":");
  while (iterator != NULL && n < 6) {
    char *fin;
    unsigned long ulval = strtoul(iterator, &fin, 16);
    if (*fin != '\0' || ulval > 0xFF) { return 0; }
    uint8_t octect = (uint8_t)ulval;
    ((uint8_t *)hwaddr)[n++] = octect;
    iterator = strtok(NULL, ":");
  }
  if (n != 6) { return 0; }
  memcpy(ifr.ifr_hwaddr.sa_data, hwaddr, 6);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) return 0;
  if (ioctl(sockfd, SIOCSIFHWADDR, &ifr)) {
    close(sockfd);
    return 0;
  }
  close(sockfd);
  return 1;
}

struct info_table {
  const char *info;
  Xen_Instance *(*get_func)(const char *);
  int (*set_func)(const char *, const char *);
};

static struct info_table if_info[] = {
    {"addr", get_addr, set_addr},
    {"netmask", get_netmask, set_netmask},
    {"broadcast", get_broadcast, set_broadcast},
    {"hwaddr", get_hwaddr, set_hwaddr},
};

static Xen_Instance *get_ifaces(Xen_Instance *self, Xen_Instance *args,
                                Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (!Xen_Function_ArgEmpty(args, kwargs)) return NULL;
  Xen_Instance *result = Xen_Vector_New();
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1) return NULL;
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_name || !ifa->ifa_addr || !(ifa->ifa_flags & IFF_UP) ||
        !(ifa->ifa_addr->sa_family == AF_INET))
      continue;
    Xen_Vector_Push(result, Xen_String_From_CString(ifa->ifa_name));
  }
  freeifaddrs(ifaddr);
  return result;
}

static Xen_Instance *get_iface_info(Xen_Instance *self, Xen_Instance *args,
                                    Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"iface", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"info", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_TUPLE,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *iface = Xen_Function_ArgBinding_Search(binding, "iface")->value;
  Xen_Instance *info = Xen_Function_ArgBinding_Search(binding, "info")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Instance *result = Xen_Vector_New();
  if (info) {
    for (Xen_size_t i = 0; i < Xen_SIZE(info); i++) {
      Xen_Instance *value = Xen_Tuple_Get_Index(info, i);
      if (!Xen_IsString(value)) return NULL;
      for (unsigned long y = 0; y < sizeof(if_info) / sizeof(struct info_table); y++) {
        if (strcmp(if_info[y].info, Xen_String_As_CString(value)) == 0) {
          Xen_Vector_Push(result, if_info[y].get_func(Xen_String_As_CString(iface)));
          break;
        } else if (y == (sizeof(if_info) / sizeof(struct info_table) - 1)) {
          return NULL;
        }
      }
    }
  } else {
    for (unsigned long i = 0; i < sizeof(if_info) / sizeof(struct info_table); i++) {
      Xen_Vector_Push(result, if_info[i].get_func(Xen_String_As_CString(iface)));
    }
  }
  return result;
}

static Xen_Instance *set_iface_info(Xen_Instance *self, Xen_Instance *args,
                                    Xen_Instance *kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"iface", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"info", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_TUPLE,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"values", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_TUPLE,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding *binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  Xen_Instance *iface = Xen_Function_ArgBinding_Search(binding, "iface")->value;
  Xen_Instance *info = Xen_Function_ArgBinding_Search(binding, "info")->value;
  Xen_Instance *values = Xen_Function_ArgBinding_Search(binding, "values")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_SIZE(info) != Xen_SIZE(values)) return NULL;
  for (Xen_size_t i = 0; i < Xen_SIZE(info); i++) {
    Xen_Instance *info_val = Xen_Tuple_Get_Index(info, i);
    Xen_Instance *val = Xen_Tuple_Get_Index(values, i);
    if (!Xen_IsString(info_val)) return NULL;
    if (!Xen_IsString(val)) return NULL;
    for (unsigned long y = 0; y < sizeof(if_info) / sizeof(struct info_table); y++) {
      if (strcmp(if_info[y].info, Xen_String_As_CString(info_val)) == 0) {
        if (!if_info[y].set_func(Xen_String_As_CString(iface),
                                 Xen_String_As_CString(val)))
          return NULL;
        break;
      } else if (y == (sizeof(if_info) / sizeof(struct info_table) - 1)) {
        return NULL;
      }
    }
  }
  return nil;
}

static struct Xen_Module_Function functions[] = {
    {"get_ifaces", get_ifaces},
    {"get_iface_info", get_iface_info},
    {"set_iface_info", set_iface_info},
    {NULL, NULL},
};

struct Xen_Module_Def *Xen_Module_ifaces_Start(void *);
struct Xen_Module_Def *Xen_Module_ifaces_Start(void *globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("ifaces", NULL, functions, NULL, NULL);
}
