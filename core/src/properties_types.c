#include <arpa/inet.h>
#include <ctype.h>
#include <ifaddrs.h>
#include <linux/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "logs.h"
#include "properties.h"
#include "properties_types.h"

int is_ip(char *addr) {
  struct sockaddr_in sin;
  int result = inet_pton(AF_INET, addr, &(sin.sin_addr));
  if (result != 1) log_add(NULL, ERROR, "IP", "{%s} no es una direccion IP", addr);
  return result == 1;
}

int to_ip(void *out_addr, char *in_addr) {
  struct in_addr addr;
  if (!inet_aton(in_addr, &addr)) {
    log_add(NULL, ERROR, "IP", "No se puedo convertir {%s} a una IP (Big-endian)",
            in_addr);
    return 0;
  }
  memcpy(out_addr, &addr.s_addr, 4);
  return 4;
}

int from_ip(char *str, void *addr, size_t size) {
  struct in_addr iaddr;
  memcpy(&iaddr.s_addr, addr, sizeof(iaddr.s_addr));
  char *result = inet_ntoa(iaddr);
  if (str == NULL) {
    log_add(NULL, ERROR, "IP", "No se pudo combertir la IP en una cadena");
    log_add_errno(NULL, ERROR, "Properties-Types");
    return 0;
  }
  strncpy(str, result, 16);
  str[15] = '\0';
  return 1;
}

int is_mac(char *hwaddr) {
  int len = strlen(hwaddr);
  if (len != 17) return 0;
  for (int i = 0; i < len; i++) {
    if ((i % 3) == 2) {
      if (hwaddr[i] != ':') {
        log_add(NULL, ERROR, "MAC", "{%s} no es una direccion MAC", hwaddr);
        return 0;
      }
    } else if (!isxdigit(hwaddr[i])) {
      log_add(NULL, ERROR, "MAC", "{%s} no es una direccion MAC");
      return 0;
    }
  }
  return 1;
}

int to_mac(void *out_hwaddr, char *in_hwaddr) {
  char str[18];
  strncpy(str, in_hwaddr, sizeof(str) - 1);
  str[sizeof(str) - 1] = '\0';
  int n = 0;
  char *iterator = strtok(str, ":");
  while (iterator != NULL && n < 6) {
    char *fin;
    unsigned long ulval = strtoul(iterator, &fin, 16);
    if (*fin != '\0' || ulval > 0xFF) {
      log_add(NULL, ERROR, "MAC", "No se puedo convertir {%s} a una MAC", in_hwaddr);
      return 0;
    }
    uint8_t octect = (uint8_t)ulval;
    ((uint8_t *)out_hwaddr)[n++] = octect;
    iterator = strtok(NULL, ":");
  }
  return n == 6 ? 6 : 0;
}

int from_mac(char *str, void *hwaddr, size_t size) {
  int result = snprintf(str, size, "%02X:%02X:%02X:%02X:%02X:%02X", ((char *)hwaddr)[0],
                        ((char *)hwaddr)[1], ((char *)hwaddr)[2], ((char *)hwaddr)[3],
                        ((char *)hwaddr)[4], ((char *)hwaddr)[5]);
  return (result >= 0) && (result < size);
}

int is_iface(char *iface) {
  struct ifaddrs *ifaddr, *ifa;
  int matched = 0;
  if (getifaddrs(&ifaddr) == -1) {
    log_add(NULL, ERROR, "IFACE",
            "No se pudieron alistar las interfaces red red disponibles");
    log_add_errno(NULL, ERROR, "Properties-types");
    return 0;
  }
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_name && strcmp(ifa->ifa_name, iface) == 0) {
      matched = 1;
      break;
    }
  }
  freeifaddrs(ifaddr);
  if (!matched) {
    log_add(NULL, ERROR, "IFACE", CIAN "{%s}" RESET " no es una interfaz valida", iface);
  }
  return matched;
}

int is_string(char *str) { return 1; }

int to_string(void *out_string, char *in_string) {
  size_t len = strlen(in_string);

  char *out = (char *)out_string;
  const char *src = in_string;
  const char *end = in_string + len;

  while (src < end) {
    if (*src == '\\') {
      src++;
      if (*src == 'n')
        *out++ = '\n';
      else if (*src == 't')
        *out++ = '\t';
      else if (*src == 'r')
        *out++ = '\r';
      else if (*src == 'b')
        *out++ = '\b';
      else if (*src == '\\')
        *out++ = '\\';
      else if (*src == '"')
        *out++ = '"';
      else if (*src == '\'')
        *out++ = '\'';
      else if (*src == 'x') {
        src++;
        int hi = ((*src >= '0' && *src <= '9')   ? *src - '0'
                  : (*src >= 'a' && *src <= 'f') ? *src - 'a' + 10
                  : (*src >= 'A' && *src <= 'F') ? *src - 'A' + 10
                                                 : -1);
        src++;
        int lo = ((*src >= '0' && *src <= '9')   ? *src - '0'
                  : (*src >= 'a' && *src <= 'f') ? *src - 'a' + 10
                  : (*src >= 'A' && *src <= 'F') ? *src - 'A' + 10
                                                 : -1);
        if (hi != -1 && lo != -1)
          *out++ = (char)((hi << 4) | lo);
        else
          *out++ = '?';
      } else {
        *out++ = '\\';
        if (*src) *out++ = *src;
      }
      src++;
    } else {
      *out++ = *src++;
    }
  }
  *out = '\0';
  return (int)(out - (char *)out_string);
}

int from_string(char *str, void *str_out, size_t size) {
  char *out = (char *)str_out;
  size_t i = 0;
  while (str[i] && (size_t)(out - (char *)str_out) < size - 2) {
    unsigned char c = (unsigned char)str[i++];
    if (c == '\n') {
      *out++ = '\\';
      *out++ = 'n';
    } else if (c == '\t') {
      *out++ = '\\';
      *out++ = 't';
    } else if (c == '\r') {
      *out++ = '\\';
      *out++ = 'r';
    } else if (c == '\b') {
      *out++ = '\\';
      *out++ = 'b';
    } else if (c == '\\') {
      *out++ = '\\';
      *out++ = '\\';
    } else if (c == '"') {
      *out++ = '\\';
      *out++ = '"';
    } else if (c == '\'') {
      *out++ = '\\';
      *out++ = '\'';
    } else if (c < 32 || c > 126) {
      if ((size_t)(out - (char *)str_out) < size - 4) {
        sprintf(out, "\\x%02X", c);
        out += 4;
      } else
        break;
    } else {
      *out++ = c;
    }
  }
  *out = '\0';
  return 1;
}

#define is_prop(name, type, code)                                                        \
  int name(prop_types prop) {                                                            \
    if (prop != type) {                                                                  \
      log_add(NULL, WARNING, "Properties-Types", "la propiedad no es del tipo " code);   \
      return 0;                                                                          \
    }                                                                                    \
    return 1;                                                                            \
  }

is_prop(is_prop_ip, IP, "IP");
is_prop(is_prop_mac, MAC, "MAC");
is_prop(is_prop_iface, IFACE, "IFACE");
