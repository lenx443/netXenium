#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "functions.h"

char *color(int index) {
  char *var_color = malloc(5);
  sprintf(var_color, "\x1b[3%dm", index);
  return var_color;
}

char *background(int index) {
  char *var_background = malloc(5);
  sprintf(var_background, "\x1b[4%dm", index);
  return var_background;
}

char *mac_to_text(uint8_t *mac) {
  char *text = malloc(17);
  sprintf(text, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5]);
  return text;
}

uint32_t get_local_ip(char *iface) {
  struct ifreq ifr;
  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket%s]", color(2), color_reset);
    perror(text_error);
    exit(EXIT_FAILURE);
  }
  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError en el ioctl%s]", color(2), color_reset);
    perror(text_error);
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  close(sockfd);
  return addr->sin_addr.s_addr;
}

uint8_t *get_local_mac(char *iface) {
  uint8_t *mac = malloc(6);
  struct ifreq ifr;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket%s]", color(2), color_reset);
    perror(text_error);
    exit(EXIT_FAILURE);
  }
  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
    char text_error[100];
    printf(text_error, "[%sError en el ioctl%s]", color(2), color_reset);
    perror(text_error);
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  close(sockfd);
  memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
  return mac;
}

uint8_t *get_mac(uint8_t *source_mac, uint32_t source_ip, uint32_t dest_ip,
                 char *iface) {
  uint8_t ether_dst[6];
  uint8_t arp_dst[6];
  memset(&ether_dst, 0xff, 6);
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
  char buffer[sizeof(eth) + sizeof(arp)];
  memcpy(buffer, &eth, sizeof(eth));
  memcpy(buffer + sizeof(eth), &arp, sizeof(arp));
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ARP);
  addr.sll_ifindex = if_nametoindex(iface);
  socklen_t addrlen = sizeof(addr);
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear el socket raw%s]", color(2),
            color_reset);
    perror(text_error);
    exit(EXIT_FAILURE);
  }
  if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr,
             addrlen) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError ao enviar packet ARP%s]", color(2),
            color_reset);
    perror(text_error);
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  memset(buffer, 0, sizeof(buffer));
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr,
               &addrlen) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al recivir el encabezado ARP%s]", color(2),
            color_reset);
    perror(text_error);
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  struct arp_header *resp = (struct arp_header *)(buffer + sizeof(buffer));
  close(sockfd);
  return resp->sha;
}

void send_arp(uint8_t *source_mac, uint32_t source_ip, uint8_t *dest_mac,
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
  char buffer[sizeof(eth) + sizeof(arp)];
  memcpy(buffer, &eth, sizeof(eth));
  memcpy(buffer + sizeof(eth), &arp, sizeof(arp));
  struct sockaddr_ll addr;
  addr.sll_family = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ARP);
  addr.sll_ifindex = if_nametoindex(iface);
  socklen_t addrlen = sizeof(addr);
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  if (sockfd < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al crear socket raw%s]", color(2),
            color_reset);
    perror(text_error);
    exit(EXIT_FAILURE);
  }
  if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr,
             addrlen) < 0) {
    char text_error[100];
    sprintf(text_error, "[%sError al enviar packete ARP%s]", color(2),
            color_reset);
    perror(text_error);
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  close(sockfd);
}
