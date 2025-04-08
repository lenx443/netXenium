#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "functions.h"

char *color(int index) {
  char *var_color = calloc(6, sizeof(char));
  if (var_color)
    sprintf(var_color, "\x1b[3%dm", index);
  return var_color;
}

char *background(int index) {
  char *var_background = calloc(6, sizeof(char));
  if (var_background)
    sprintf(var_background, "\x1b[4%dm", index);
  return var_background;
}

char *mac_to_text(uint8_t *mac) {
  char *text = calloc(18, sizeof(char));
  if (text)
    sprintf(text, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
  return text;
}

int get_local_ip(uint32_t *local_ip, char *iface) {
  char *cl = color(2);
  struct ifreq ifr;
  memset(&ifr, 0x00, sizeof(struct ifreq));
  memset(local_ip, 0x00, sizeof(uint32_t));
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket%s]", cl, color_reset);
    perror(text_error);
    free(cl);
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError en el ioctl%s]", cl, color_reset);
    perror(text_error);
    close(sockfd);
    free(cl);
    return 0;
  }
  close(sockfd);
  free(cl);
  memcpy(local_ip, &addr->sin_addr.s_addr, sizeof(uint32_t));
  return 1;
}

int get_local_mac(uint8_t *local_mac, char *iface) {
  char *cl = color(2);
  struct ifreq ifr;
  memset(&ifr, 0x00, sizeof(struct ifreq));
  memset(local_mac, 0x00, sizeof(uint32_t));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket%s]", cl, color_reset);
    perror(text_error);
    free(cl);
    return 0;
  }
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
    char text_error[100];
    printf(text_error, "[%sError en el ioctl%s]", cl, color_reset);
    perror(text_error);
    close(sockfd);
    free(cl);
    return 0;
  }
  close(sockfd);
  free(cl);
  memcpy(local_mac, ifr.ifr_hwaddr.sa_data, 6);
  return 1;
}

int get_mac(uint8_t *mac, uint8_t *source_mac, uint32_t source_ip,
            uint32_t dest_ip, char *iface) {
  char *cl = color(2);
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
    char text_error[100];
    sprintf(text_error, "[%sError al crear el socket raw%s]", cl, color_reset);
    perror(text_error);
    free(cl);
    return 0;
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0,
             (struct sockaddr *)&addr, addrlen) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError ao enviar packet ARP%s]", cl, color_reset);
    perror(text_error);
    close(sockfd);
    free(cl);
    return 0;
  }
  memset(send_buffer, 0, sizeof(send_buffer));
  close(sockfd);

  char pcap_errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *pcap_handle = pcap_open_live(iface, BUFSIZ, 0, 100, pcap_errbuf);
  if (!pcap_handle) {
    printf("[%sError al recivir respuesta%s]: %s\n", cl, color_reset,
           pcap_errbuf);
    free(cl);
    return 0;
  }
  char bpf_filter[100];
  snprintf(bpf_filter, 100, "arp[6:2] = 2 and arp[14:4] = 0x%08x",
           htonl(dest_ip));
  struct bpf_program fp;
  if (pcap_compile(pcap_handle, &fp, bpf_filter, 0, PCAP_NETMASK_UNKNOWN) < 0) {
    printf("[%sError al configurar filtro de red%s]: %s\n", cl, color_reset,
           pcap_geterr(pcap_handle));
    pcap_close(pcap_handle);
    free(cl);
    return 0;
  }
  if (pcap_setfilter(pcap_handle, &fp) < 0) {
    printf("[%sError aplicando filtro de red%s]: %s\n", cl, color_reset,
           pcap_geterr(pcap_handle));
    pcap_close(pcap_handle);
    free(cl);
    return 0;
  }
  struct pcap_pkthdr header;
  const u_char *packet = pcap_next(pcap_handle, &header);
  if (!packet) {
    printf("[%sError al capturar paquete%s]: %s", cl, color_reset,
           pcap_geterr(pcap_handle));
    pcap_close(pcap_handle);
    free(cl);
    return 0;
  }
  struct arp_header resp =
      *(struct arp_header *)(packet + sizeof(struct ethhdr));
  memcpy(mac, resp.sha, 6);
  pcap_close(pcap_handle);
  free(cl);
  return 1;
}

void send_arp(uint8_t *source_mac, uint32_t source_ip, uint8_t *dest_mac,
              uint32_t dest_ip, char *iface) {
  char *cl = color(2);
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
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket raw%s]", cl, color_reset);
    perror(text_error);
    free(cl);
    exit(EXIT_FAILURE);
  }
  if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0,
             (struct sockaddr *)&addr, addrlen) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al enviar packete ARP%s]", cl, color_reset);
    perror(text_error);
    close(sockfd);
    free(cl);
    exit(EXIT_FAILURE);
  }
  close(sockfd);
  free(cl);
}
