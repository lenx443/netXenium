#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <linux/if_arp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define color_reset "\x1b[0m"

char * color(int index);
char * background(int index);
char * mac_to_text(uint8_t * mac);
uint32_t get_local_ip(char * iface);
int in(int c1, int c2[], int lengeth);
uint8_t * get_local_mac(char * iface);
uint8_t * get_mac(uint8_t * local_mac,uint32_t local_ip, uint32_t dest_ip, char * iface);
void filter(uint8_t * local_mac, uint8_t * router_mac, int protos[], int lengeth, char * iface);

struct arp_header {
        uint16_t hardware_type;
        uint16_t protocol_type;
        uint8_t hardware_len;
        uint8_t protocol_len;
        uint16_t op;
        uint8_t sha[6];
        uint32_t spa;
        uint8_t tha[6];
        uint32_t tpa;
} __attribute__((packed));

int main(int argc, char * argv[]) {
        printf("%sNet Filter (C) 2024 X-Leamsi%s\n\n",
        color(4), color_reset);
        printf("[%s1%s] %sFiltrar ARP%s\n",
        color(3), color_reset, color(1), color_reset);
        printf("[%s2%s] %sFiltrar IP%s\n",
        color(3), color_reset, color(1), color_reset);
        printf("[%s3%s] %sFiltrar UDP%s\n",
        color(3), color_reset, color(1), color_reset);
        printf("[%s4%s] %sFiltrar TCP%s\n",
        color(3), color_reset, color(1), color_reset);
        printf("[%s5%s] %sFiltrar ICMP%s\n",
        color(3), color_reset, color(1), color_reset);
        printf("\n[%sCuntos protocolos desa filtrar%s]: ",
        color(2), color_reset);
        int count;
        scanf("%d", &count);
        if ((count <= 0) || (count > 5)) {
                fprintf(stderr, "(%sEl numro que proporciono no es valido%s)",
                background(1), color_reset);
                exit(EXIT_FAILURE);
        }
        int protos[count];
        for(int index = 0; index < count; index++) {
                printf("[%sProtocolo #%d a filtrar%s]: ",
                color(2), index + 1, color_reset);
                int value;
                scanf("%d", &value);
                if      (value == 1) protos[index] = ETH_P_ARP;
                else if (value == 2) protos[index] = ETH_P_IP;
                else if (value == 3) protos[index] = IPPROTO_UDP;
                else if (value == 4) protos[index] = IPPROTO_TCP;
                else if (value == 5) protos[index] = IPPROTO_ICMP;
        }
        printf("[%sInterfaz%s]: ",
        color(2), color_reset);
        char iface[IFNAMSIZ];
        scanf("%s", iface);
        printf("[%sIP del Router%s]: ",
        color(2), color_reset);
        char str_router_ip[15];
        scanf("%s", str_router_ip);
        uint32_t router_ip = inet_addr(str_router_ip);

        uint32_t local_ip = get_local_ip(iface);

        uint8_t * local_mac = get_local_mac(iface);

        uint8_t * router_mac = get_mac(local_mac, local_ip, router_ip, iface);

        printf("\n(%s***%s) IP de esta maquina: %s%s%s%s; MAC de esta maquina %s%s%s%s;\n",
        color(4), color_reset, background(6), color(0), inet_ntoa(*(struct in_addr*)&local_ip), color_reset,
        background(6), color(0), mac_to_text(local_mac), color_reset);

        printf("\n(%s***%s) IP del Router: %s%s%s%s; MAC del Router: %s%s%s%s;\n",
        color(4), color_reset, background(6), color(0), inet_ntoa(*(struct in_addr*)&router_ip), color_reset,
        background(6), color(0), mac_to_text(router_mac), color_reset);

        printf("\n(%s***%s) %sFiltro iniciado%s\n",
        color(4), color_reset, color(3), color_reset);

        filter(local_mac, router_mac, protos, count, iface);
}

char * color(int index) {
        char * text_color = malloc(6);
        sprintf(text_color, "\x1b[3%dm", index);
        return text_color;
}

char * background(int index) {
        char * text_background = malloc(6);
        sprintf(text_background, "\x1b[4%dm", index);
        return text_background;
}

int in(int c1, int c2[], int lengeth) {
        for (int x = 0; c2[x] < lengeth; x++) {
                if (c1 == c2[x]) {
                        return 1;
                }
        }
        return 0;
}

char * mac_to_text(uint8_t * mac) {
        char * text = malloc(17);
        sprintf(text, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return text;
}

uint32_t get_local_ip(char * iface) {
        struct ifreq ifr;
        struct sockaddr_in * addr = (struct sockaddr_in*)&ifr.ifr_addr;
        strncpy(ifr.ifr_name, iface, IFNAMSIZ);
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al crear socket%s]: ",
                color(2), color_reset);
                perror("socket");
                exit(EXIT_FAILURE);
        }
        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError en el ioctl%s]: ",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        close(sockfd);
        return addr->sin_addr.s_addr;
}

uint8_t * get_local_mac(char * iface) {
        uint8_t * mac = malloc(6);
        struct ifreq ifr;
        strncpy(ifr.ifr_name, iface, IFNAMSIZ);
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al crear socket%s]: ",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError en el ioctl%s]: ",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        close(sockfd);
        memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
        return mac;
}

uint8_t * get_mac(uint8_t * local_mac, uint32_t local_ip, uint32_t dest_ip, char * iface) {
        uint8_t ether_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        uint8_t arp_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        struct ethhdr eth;
        memcpy(eth.h_source, local_mac, 6);
        memcpy(eth.h_dest, ether_mac, 6);
        eth.h_proto = htons(ETH_P_ARP);
        struct arp_header arp;
        arp.hardware_type = htons(ARPHRD_ETHER);
        arp.protocol_type = htons(ETH_P_IP);
        arp.hardware_len = 6;
        arp.protocol_len = 4;
        arp.op = htons(1);
        memcpy(arp.sha, local_mac, 6);
        arp.spa = local_ip;
        memcpy(arp.tha, arp_mac, 6);
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
                sprintf(text_error, "[%sError al crear socket%s]: ",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, addrlen) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al enviar paquete ARP%s]: ",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addrlen) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al obtener paquete ARP%s]: ",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        struct arp_header resp = *((struct arp_header*)(buffer + sizeof(eth)));
        uint8_t * mac = malloc(6);
        memcpy(mac, resp.sha, 6);
        return mac;
}

void filter(uint8_t * local_mac, uint8_t * router_mac, int protos[], int lengeth, char * iface) {
        struct sockaddr_ll addr;
        addr.sll_family = AF_PACKET;
        addr.sll_protocol = htons(ETH_P_ALL);
        addr.sll_ifindex = if_nametoindex(iface);
        socklen_t addrlen = sizeof(addr);
        int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sockfd < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al socket%s]: ",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        while (1) {
                char buffer[65535];
                if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addrlen) < 0) {
                        char text_error[100];
                        sprintf(text_error, "[%sError al recivir paquete%s]: ",
                        color(2), color_reset);
                        perror(text_error);
                        close(sockfd);
                        exit(EXIT_FAILURE);
                }
                struct ethhdr eth = *((struct ethhdr*)buffer);
                if (((uint8_t*)eth.h_source != router_mac) && ((uint8_t*)eth.h_dest == local_mac)) {
                        if (in((int)ntohs(eth.h_proto), protos, lengeth) == 0) {
                                if (eth.h_proto == htons(ETH_P_IP)) {
                                        struct iphdr ip = *((struct iphdr*)(buffer + sizeof(eth)));
                                        if (in((int)ip.protocol, protos, lengeth) == 1) {
                                                continue;
                                        }
                                        if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, addrlen) < 0) {
                                                char text_error[100];
                                                sprintf(text_error, "[%sError al enviar paquete%s]: ",
                                                color(2), color_reset);
                                                perror(text_error);
                                                close(sockfd);
                                                exit(EXIT_FAILURE);
                                        }
                                }
                        }
                }
        }
}