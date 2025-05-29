#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "colors.h"
#include "functions.h"
#include "list.h"
#include "logs.h"

int strip_ansi_escape_strlen(const char *str) {
  int len = 0;
  int in_escape = 0;

  for (int i = 0; str[i]; i++) {
    if (str[i] == '\033') {
      in_escape = 1;
      continue;
    }
    if (in_escape) {
      if (str[i] == 'm') { in_escape = 0; }
      continue;
    }
    len++;
  }
  return len;
}

void strip_ansi_escape(char *str) {
  char *src = str;
  char *dst = str;
  while (*src) {
    if (*src == '\x1B' && *(src + 1) == '[') {
      src += 2;
      while (*src && !((*src >= '@' && *src <= '~'))) {
        src++;
      }
      if (*src) src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

int get_local_ip(uint32_t *local_ip, char *iface) {
  struct ifreq ifr = {0};
  memset(local_ip, 0x00, sizeof(uint32_t));
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "GetLocalIp", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "GetLocalIp", "Error al obtener la IP local");
    log_add(NULL, ERROR, "GetLocalIp", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalIp");
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "GetLocalIp", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetLocalIp", "Error al obtener la IP local");
    log_add(NULL, ERROR, "GetLocalIp", "Interfaz -> {%s}", iface);
    log_add_errno(NULL, ERROR, "GetLocalIp");
    return 0;
  }
  close(sockfd);
  memcpy(local_ip, &addr->sin_addr.s_addr, sizeof(uint32_t));
  return 1;
}

int get_local_netmask(uint32_t *local_netmask, char *iface) {
  struct ifreq ifr = {0};
  memset(local_netmask, 0x00, sizeof(uint32_t));
  struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "GetLocalNetmask", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "GetLocalNetmask", "Error al obtener la mascara de red local");
    log_add(NULL, ERROR, "GetLocalNetmask", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalNetmask");
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "GetLocalNetmask", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetLocalNetmask", "Error al obtener la mascara de red local");
    log_add(NULL, ERROR, "GetLocalNetmask", "Interfaz -> {%s}", iface);
    log_add_errno(NULL, ERROR, "GetLocalNetmask");
    return 0;
  }
  close(sockfd);
  memcpy(local_netmask, &netmask->sin_addr.s_addr, sizeof(uint32_t));
  return 1;
}

int get_local_broadcast(uint32_t *local_broad, char *iface) {
  struct ifreq ifr = {0};
  memset(local_broad, 0x00, sizeof(uint32_t));
  struct sockaddr_in *broad = (struct sockaddr_in *)&ifr.ifr_broadaddr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "GetLocalBroadcast", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "GetLocalBroadcast", "Error al obtener el broadcast de red local");
    log_add(NULL, ERROR, "GetLocalBroadcast", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalBroadcast");
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "GetLocalBroadcast", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetLocalBroadcast", "Error al obtener el broadcast de red local");
    log_add(NULL, ERROR, "GetLocalBroadcast", "Interfaz -> {%s}", iface);
    log_add_errno(NULL, ERROR, "GetLocalBroadcast");
    return 0;
  }
  close(sockfd);
  memcpy(local_broad, &broad->sin_addr.s_addr, sizeof(uint32_t));
  return 1;
}

int get_local_mac(uint8_t *local_mac, char *iface) {
  struct ifreq ifr = {0};
  memset(local_mac, 0x00, sizeof(uint32_t));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "GetLocalMac", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "GetLocalMac", "Error al obtener la MAC local");
    log_add(NULL, ERROR, "GetLocalIp", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalIp");
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "GetLocalMac", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetLocalMac", "Error al obtener la MAC local");
    log_add(NULL, ERROR, "GetLocalMac", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalMac");
    return 0;
  }
  close(sockfd);
  memcpy(local_mac, ifr.ifr_hwaddr.sa_data, 6);
  return 1;
}

int set_local_ip(uint32_t local_addr, char *iface) {
  struct ifreq ifr = {0};
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in saddr = {
      .sin_addr.s_addr = local_addr,
      .sin_family = AF_INET,
  };
  memcpy(&ifr.ifr_addr, &saddr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "SetLocalAddr", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "SetLocalAddr", "Error al cambiar la IP local");
    log_add(NULL, ERROR, "SetLocalAddr", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalAddr");
    return 0;
  }
  if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
    log_add(NULL, ERROR, "SetLocalAddr", "{ioctl}(fd, op, ..)");
    log_add(NULL, ERROR, "SetLocalAddr", "Error al cambiar la IP local");
    log_add(NULL, ERROR, "SetLocalAddr", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalAddr");
    return 0;
  }
  return 1;
}

int set_local_netmask(uint32_t local_netmask, char *iface) {
  struct ifreq ifr = {0};
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in saddr = {
      .sin_addr.s_addr = local_netmask,
      .sin_family = AF_INET,
  };
  memcpy(&ifr.ifr_netmask, &saddr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "SetLocalNetmask", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "SetLocalNetmask", "Error al cambiar la mascara de red local");
    log_add(NULL, ERROR, "SetLocalNetmask", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalNetmask");
    return 0;
  }
  if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
    log_add(NULL, ERROR, "SetLocalNetmask", "{ioctl}(fd, op, ..)");
    log_add(NULL, ERROR, "SetLocalNetmask", "Error al cambiar la mascara de red local");
    log_add(NULL, ERROR, "SetLocalNetmask", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalNetmask");
    return 0;
  }
  return 1;
}

int set_local_broadcast(uint32_t local_broad, char *iface) {
  struct ifreq ifr = {0};
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  struct sockaddr_in saddr = {
      .sin_addr.s_addr = local_broad,
      .sin_family = AF_INET,
  };
  memcpy(&ifr.ifr_broadaddr, &saddr, sizeof(struct sockaddr));
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "SetLocalBroadcast", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "SetLocalBroadcast", "Error al cambiar el BROADCAST local");
    log_add(NULL, ERROR, "SetLocalBroadcast", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalBroadcast");
    return 0;
  }
  if (ioctl(sockfd, SIOCSIFBRDADDR, &ifr) < 0) {
    log_add(NULL, ERROR, "SetLocalBroadcast", "{ioctl}(fd, op, ..)");
    log_add(NULL, ERROR, "SetLocalBroadcast", "Error al cambiar el BROADCAST local");
    log_add(NULL, ERROR, "SetLocalBroadcast", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SetLocalBroadcast");
    return 0;
  }
  return 1;
}

int set_local_mac(uint8_t *local_mac, char *iface) {
  struct ifreq ifr = {0};
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  memcpy(ifr.ifr_hwaddr.sa_data, local_mac, 6);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    log_add(NULL, ERROR, "GetLocalMac", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "GetLocalMac", "Error al cambiar la MAC local");
    log_add(NULL, ERROR, "GetLocalMac", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalIp");
    return 0;
  }
  if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "GetLocalMac", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetLocalMac", "Error al cambiar la MAC local");
    log_add(NULL, ERROR, "GetLocalMac", "Interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "GetLocalMac");
    return 0;
  }
  close(sockfd);
  return 1;
}

LIST_ptr get_ifaces_list() {
  LIST_ptr iface_list = list_new();
  if (iface_list == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "GetIfaceLists", "No se pudieron alistar las interfaces de red disponibles");
    return NULL;
  }
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1) {
    log_add(NULL, ERROR, "GetIfaceLists", "{getifaddrs}(list_ptr)");
    log_add(NULL, ERROR, "GetIfaceLists", "No se pudieron alistar las interfaces disponibles");
    log_add_errno(NULL, ERROR, "GetIfaceLists");
    return NULL;
  }
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_name || !ifa->ifa_addr || !(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_addr->sa_family == AF_INET))
      continue;
    if (!list_push_back_string_node(iface_list, ifa->ifa_name)) {
      DynSetLog(NULL);
      log_add(NULL, ERROR, "GetIfaceLists", "No se pudo guardar la lista de interfaces");
      return NULL;
    }
  }
  freeifaddrs(ifaddr);
  return iface_list;
}

int send_arp_request(uint8_t *source_mac, uint32_t source_ip, uint32_t dest_ip, char *iface) {
  uint8_t ether_dst[6];
  uint8_t arp_dst[6];
  memset(ether_dst, 0xff, 6);
  memset(arp_dst, 0x00, 6);
  struct ethhdr eth;
  memcpy(eth.h_source, source_mac, 6);
  memcpy(eth.h_dest, ether_dst, 6);
  eth.h_proto = htons(ETH_P_ARP);
  struct arp_header arp;
  arp.hardware_type = htons(ARPHRD_ETHER);
  arp.protocol_type = htons(0x0800);
  arp.hardware_len = 6;
  arp.protocol_len = 4;
  arp.op = htons(1);
  memcpy(arp.sha, source_mac, 6);
  arp.spa = source_ip;
  memcpy(arp.tha, arp_dst, 6);
  arp.tpa = dest_ip;
  char send_buffer[sizeof(eth) + sizeof(arp)];
  memcpy(send_buffer, &eth, sizeof(eth));
  memcpy(send_buffer + sizeof(eth), &arp, sizeof(arp));
  struct sockaddr_ll addr;
  memset(&addr, 0x00, sizeof(addr));
  addr.sll_family = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ARP);
  addr.sll_ifindex = if_nametoindex(iface);
  addr.sll_halen = ETH_ALEN;
  memcpy(addr.sll_addr, ether_dst, ETH_ALEN);
  socklen_t addrlen = sizeof(addr);
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if (sockfd < 0) {
    log_add(NULL, ERROR, "SendArpRequest", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "SendArpRequest", "Error al crear un socket crudo");
    log_add(NULL, ERROR, "SendArpRequest", "Interfaz " CIAN "%s" RESET, iface);
    log_add_errno(NULL, ERROR, "SendArpRequest");
    return 0;
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&addr, addrlen) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "SendArpRequest", "{sendto}(fd, buf, n, flag, dst_addr, dst_addr_length)");
    log_add(NULL, ERROR, "SendArpRequest", "Error al enviar el paquete ARP");
    log_add(NULL, ERROR, "SendArpRequest", "Interfaz " CIAN "%s" RESET, iface);
    log_add_errno(NULL, ERROR, "SendArpRequest");
    return 0;
  }
  close(sockfd);
  return 1;
}

int send_arp_reply(uint8_t *source_mac, uint32_t source_ip, uint8_t *dest_mac, uint32_t dest_ip, char *iface) {
  struct ethhdr eth;
  memcpy(eth.h_source, source_mac, 6);
  memcpy(eth.h_dest, dest_mac, 6);
  eth.h_proto = htons(ETH_P_ARP);
  struct arp_header arp;
  arp.hardware_type = htons(ARPHRD_ETHER);
  arp.protocol_type = htons(0x0800);
  arp.hardware_len = 6;
  arp.protocol_len = 4;
  arp.op = htons(2);
  memcpy(arp.sha, source_mac, 6);
  arp.spa = source_ip;
  memcpy(arp.tha, dest_mac, 6);
  arp.tpa = dest_ip;
  char send_buffer[sizeof(eth) + sizeof(arp)];
  memcpy(send_buffer, &eth, sizeof(eth));
  memcpy(send_buffer + sizeof(eth), &arp, sizeof(arp));
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ARP);
  addr.sll_ifindex = if_nametoindex(iface);
  socklen_t addrlen = sizeof(addr);
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if (sockfd < 0) {
    log_add(NULL, ERROR, "SendArpReply", "{socket}(af, type, protocol)");
    log_add(NULL, ERROR, "SendArpReply", "Error al crear un socket crudo");
    log_add(NULL, ERROR, "SendArpReply", "interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SendArpReply");
    return 0;
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&addr, addrlen) < 0) {
    close(sockfd);
    log_add(NULL, ERROR, "SendArpReply", "{sendto}(fd, buf, n, flag, dst_addr, dst_addr_length)");
    log_add(NULL, ERROR, "SendArpReply", "Error al enviar el paquete ARP");
    log_add(NULL, ERROR, "SendArpReply", "interfaz -> " CIAN "{%s}" RESET, iface);
    log_add_errno(NULL, ERROR, "SendArpReply");
    return 0;
  }
  close(sockfd);
  return 1;
}
