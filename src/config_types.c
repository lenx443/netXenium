#include <arpa/inet.h>
#include <bits/in_addr.h>
#include <ctype.h>
#include <ifaddrs.h>
#include <linux/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "config_types.h"
#include "error_codes.h"
#include "errs.h"

int is_iface(char *iface) {
  struct ifaddrs *ifaddr, *ifa;
  int matched = 0;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return 0;
  }
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_name && strcmp(ifa->ifa_name, iface) == 0) {
      matched = 1;
      break;
    }
  }
  freeifaddrs(ifaddr);
  if (!matched)
    code_error = ERROR_TYPE_IFACE;
  return matched;
}

int is_addr(char *addr) {
  struct sockaddr_in sin;
  int result = inet_pton(AF_INET, addr, &(sin.sin_addr));
  if (result != 1)
    code_error = ERROR_TYPE_IP;
  return result == 1;
}

int to_addr(uint32_t *out_addr, char *in_addr) {
  struct in_addr addr;
  if (!inet_aton(in_addr, &addr)) {
    code_error = ERROR_TYPE_IP;
    return 0;
  }
  *out_addr = addr.s_addr;
  return 1;
}

int from_addr(char *str, uint32_t addr) {
  struct in_addr iaddr;
  iaddr.s_addr = addr;
  char *result = inet_ntoa(iaddr);
  if (str == NULL)
    return 0;
  strncpy(str, result, 16);
  str[15] = '\0';
  return 1;
}

int is_hwaddr(char *hwaddr) {
  int len = strlen(hwaddr);
  if (len != 17)
    return 0;
  for (int i = 0; i < len; i++) {
    if ((i % 3) == 2) {
      if (hwaddr[i] != ':') {
        code_error = ERROR_TYPE_MAC;
        return 0;
      }
    } else if (!isxdigit(hwaddr[i])) {
      code_error = ERROR_TYPE_MAC;
      return 0;
    }
  }
  return 1;
}

int to_hwaddr(uint8_t *out_hwaddr, char *in_hwaddr) {
  char str[18];
  strncpy(str, in_hwaddr, sizeof(str));
  str[17] = '\0';
  int n = 0;
  char *iterator = strtok(str, ":");
  while (iterator != NULL & n < 6) {
    char *fin;
    unsigned long ulval = strtoul(iterator, &fin, 16);
    if (*fin != '\n' || ulval > 0xFF) {
      code_error = ERROR_TYPE_MAC;
      return 0;
    }
    uint8_t octect = (uint8_t)ulval;
    out_hwaddr[n++] = octect;
    iterator = strtok(NULL, ":");
  }
  return 1;
}

int from_hwaddr(char *str, size_t size, uint8_t *hwaddr) {
  int result = snprintf(str, size, "%02X:%02X:%02X:%02X:%02X:%02X", hwaddr[0],
                        hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
  return (result >= 0) && (result < size);
}

#define is_prop(name, type, code)                                              \
  int name(config_types prop) {                                                \
    if (prop != type) {                                                        \
      code_error = code;                                                       \
      return 0;                                                                \
    }                                                                          \
    return 1;                                                                  \
  }

is_prop(is_prop_ip, IP, ERROR_PROP_IP);
is_prop(is_prop_mac, MAC, ERROR_PROP_MAC);
is_prop(is_prop_iface, IFACE, ERROR_PROP_IFACE);
