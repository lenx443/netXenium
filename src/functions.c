/*
 * En este archivo se definen una
 * gran parte de las fubciones
 * empleadas en este programa
 * formando asi una parte de su
 * API interna
 */

#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include "error_codes.h"
#include "errs.h"
#include "functions.h"

/*
 * Esta funcion permite obtener
 * la direccion IP de esta maquina
 * en una interfaz de red empleando
 * la llamada al sistema `ioctl`.
 * Si se produce un error lo puntualiza
 * en la global `code_error`.
 */
int get_local_ip(uint32_t *local_ip, char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0x00, sizeof(struct ifreq));
  memset(local_ip, 0x00, sizeof(uint32_t));
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    if (errno == EMFILE || errno == ENFILE)
      code_error = ERROR_SOCKET_FD_LIMIT;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_SOCKET_PERMISSIONS;
    else
      code_error = ERROR_SOCKET;
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    close(sockfd);
    if (errno == EBADF)
      code_error = ERROR_IOCTL_FD_INVALID;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_IOCTL_PERMISSIONS;
    else if (errno == ENOMEM || errno == EFAULT)
      code_error = ERROR_IOCTL_MEMORY;
    else
      code_error = ERROR_IOCTL;
    return 0;
  }
  close(sockfd);
  memcpy(local_ip, &addr->sin_addr.s_addr, sizeof(uint32_t));
  return 1;
}

/*
 * Esta funcion permite obtener
 * la direccion MAC de esta maquina
 * en una interfaz de red empleando
 * la llamada al sistema `ioctl`.
 * Si se produce un error lo puntualiza
 * en la global `code_error`.
 */
int get_local_mac(uint8_t *local_mac, char *iface) {
  struct ifreq ifr;
  memset(&ifr, 0x00, sizeof(struct ifreq));
  memset(local_mac, 0x00, sizeof(uint32_t));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    if (errno == EMFILE || errno == ENFILE)
      code_error = ERROR_SOCKET_FD_LIMIT;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_SOCKET_PERMISSIONS;
    else
      code_error = ERROR_SOCKET;
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
    close(sockfd);
    if (errno == EBADF)
      code_error = ERROR_IOCTL_FD_INVALID;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_IOCTL_PERMISSIONS;
    else if (errno == ENOMEM || errno == EFAULT)
      code_error = ERROR_IOCTL_MEMORY;
    else
      code_error = ERROR_IOCTL;
    return 0;
  }
  close(sockfd);
  memcpy(local_mac, ifr.ifr_hwaddr.sa_data, 6);
  return 1;
}

/*
 * Esta funcion construye y envia un
 * paquete ARP request, usando como
 * direccion MAC ethernet la MAC del
 * gateway y como direccion MAC arp
 * una MAC llena de ceros (para
 * decubrir direcciones MAC).
 * Los errores se puntualizan en la
 * global `code_error`.
 */
int send_arp_request(uint8_t *source_mac, uint32_t source_ip, uint32_t dest_ip,
                     char *iface) {
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
    if (errno == EMFILE || errno == ENFILE)
      code_error = ERROR_SOCKET_RAW_FD_LIMIT;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_SOCKET_RAW_PERMISSIONS;
    else
      code_error = ERROR_SOCKET_RAW;
    return 0;
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0,
             (struct sockaddr *)&addr, addrlen) < 0) {
    close(sockfd);
    if (errno == ENOBUFS || errno == ENOMEM)
      code_error = ERROR_SEND_BUFFER;
    else if (errno == EINTR)
      code_error = ERROR_SEND_INTERRUPT;
    else if (errno == ENETUNREACH || errno == EHOSTUNREACH ||
             errno == ECONNREFUSED)
      code_error = ERROR_SEND_REFUSED;
    else if (errno == EPIPE || errno == ECONNREFUSED)
      code_error = ERROR_SEND_CONNECT;
    else if (errno == EACCES)
      code_error = ERROR_SEND_ACCESS;
    else
      code_error = ERROR_SEND;
    return 0;
  }
  close(sockfd);
  return 1;
}

/*
 * Esta funcion construye y envia
 * un paquete ARP reply.
 * Los errores se puntualizan
 * en la global `code_error`.
 */
int send_arp_reply(uint8_t *source_mac, uint32_t source_ip, uint8_t *dest_mac,
                   uint32_t dest_ip, char *iface) {
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
    if (errno == EMFILE || errno == ENFILE)
      code_error = ERROR_SOCKET_RAW_FD_LIMIT;
    else if (errno == EACCES || errno == EPERM)
      code_error = ERROR_SOCKET_RAW_PERMISSIONS;
    else
      code_error = ERROR_SOCKET_RAW;
    return 0;
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0,
             (struct sockaddr *)&addr, addrlen) < 0) {
    close(sockfd);
    if (errno == ENOBUFS || errno == ENOMEM)
      code_error = ERROR_SEND_BUFFER;
    else if (errno == EINTR)
      code_error = ERROR_SEND_INTERRUPT;
    else if (errno == ENETUNREACH || errno == EHOSTUNREACH ||
             errno == ECONNREFUSED)
      code_error = ERROR_SEND_REFUSED;
    else if (errno == EPIPE || errno == ECONNREFUSED)
      code_error = ERROR_SEND_CONNECT;
    else if (errno == EACCES)
      code_error = ERROR_SEND_ACCESS;
    else
      code_error = ERROR_SEND;
    return 0;
  }
  close(sockfd);
  return 1;
}
