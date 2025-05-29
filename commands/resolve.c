#include <linux/if_ether.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "commands.h"
#include "functions.h"
#include "list.h"
#include "logs.h"
#include "pcap_wrapper.h"
#include "properties.h"
#include "properties_types.h"

#define NAME "resolve"

static pcap_t *pcap_handle;
static uint8_t local_mac[6];
static uint8_t result_mac[6];
static int keyboard_interrupt = 0;

static void keyboard_interrupt_handle(int sign) {
  pcap_breakloop(pcap_handle);
  keyboard_interrupt = 1;
}

static int fn_resolve(LIST_ptr args) {
  if (list_size(*args) < 2) {
    fprintf(stderr, "Uso: " NAME " [propiedad de entrada (IP)] [propiedad de salida(MAC)]\n");
    return EXIT_FAILURE;
  }

  prop_struct *iface = prop_reg_value("LOCAL_IFACE", *prop_register);
  if (iface == NULL) {
    log_add(NULL, ERROR, NAME, "Error al obtener la interfaz local");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  uint32_t local_ip;
  if (!get_local_ip(&local_ip, iface->value)) {
    log_add(NULL, ERROR, NAME, "No se pudo obtener la direccion IP local");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  if (!get_local_mac(local_mac, iface->value)) {
    log_add(NULL, ERROR, NAME, "No se pudo obtener la direccion MAC local");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  NODE_ptr in_prop = list_index_get(1, *args);
  if (in_prop == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se pudo acceder a [in prop]");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  char *in_prop_value = (char *)in_prop->point;
  char *out_prop_value = NULL;
  prop_struct *out_prop_cfg = NULL;
  uint32_t dest_ip;

  if (strcmp(in_prop_value, "target") == 0) {
    prop_struct *target_prop_cfg = prop_reg_value("TARGET_ADDR", *prop_register);
    if (target_prop_cfg == NULL) {
      log_add(NULL, ERROR, NAME, "Propiedad TARGET_ADDR requerida");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    } else if (target_prop_cfg->type != IP) {
      log_add(NULL, ERROR, NAME, "La propiedad TARGET_ADDR no es una IP");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    out_prop_value = "TARGET_HWADDR";
    out_prop_cfg = prop_reg_value("TARGET_HWADDR", *prop_register);
    if (out_prop_cfg == NULL) {
      if (!prop_reg_add(prop_register, "TARGET_HWADDR", "", MAC)) {
        log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad TARGET_HWADDR");
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      out_prop_cfg = prop_reg_value("TARGET_HWADDR", *prop_register);
    } else if (out_prop_cfg->type != MAC) {
      log_add(NULL, ERROR, NAME, "La propiedad TARGET_HWADDR no es una MAC");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    to_ip(&dest_ip, target_prop_cfg->value);

  } else if (strcmp(in_prop_value, "router") == 0) {
    prop_struct *router_prop_cfg = prop_reg_value("ROUTER_ADDR", *prop_register);
    if (router_prop_cfg == NULL) {
      log_add(NULL, ERROR, NAME, "Propiedad ROUTER_ADDR requerida");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    } else if (router_prop_cfg->type != IP) {
      log_add(NULL, ERROR, NAME, "La propiedad ROUTER_ADDR no es una IP");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    out_prop_value = "ROUTER_HWADDR";
    out_prop_cfg = prop_reg_value("ROUTER_HWADDR", *prop_register);
    if (out_prop_cfg == NULL) {
      if (!prop_reg_add(prop_register, "ROUTER_HWADDR", "", MAC)) {
        log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad ROUTER_HWADDR");
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      out_prop_cfg = prop_reg_value("ROUTER_HWADDR", *prop_register);
    } else if (out_prop_cfg->type != MAC) {
      log_add(NULL, ERROR, NAME, "La propiedad ROUTER_HWADDR no es una MAC");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    to_ip(&dest_ip, router_prop_cfg->value);

  } else {
    prop_struct *in_prop_cfg = prop_reg_value(in_prop_value, *prop_register);
    if (in_prop_cfg == NULL) {
      log_add(NULL, ERROR, NAME, "No se encontro la propiedad de entrada");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }

    NODE_ptr out_prop = list_index_get(2, *args);
    if (out_prop == NULL) {
      DynSetLog(NULL);
      log_add(NULL, ERROR, NAME, "Error al obtener [out prop]");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    out_prop_value = (char *)out_prop->point;
    out_prop_cfg = prop_reg_value(out_prop_value, *prop_register);
    if (out_prop_cfg == NULL) {
      log_add(NULL, ERROR, NAME, "No se encontro la propiedad de salida");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    if (!is_prop_ip(in_prop_cfg->type)) {
      log_add(NULL, ERROR, NAME, "La propiedad de entrada no es correcta");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
    if (!is_prop_mac(out_prop_cfg->type)) {
      log_add(NULL, ERROR, NAME, "La propiedad de salida no es correcta");
      log_show_and_clear(NULL);
      EXIT_FAILURE;
    }

    if (!to_ip(&dest_ip, in_prop_cfg->value)) {
      log_add(NULL, ERROR, NAME, "La IP de entrada es invalida");
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
  }

  if (!send_arp_request(local_mac, local_ip, dest_ip, iface->value)) {
    log_add(NULL, ERROR, NAME, "Error al enviar paquete ARP");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_handle = NULL;
  if (!wrapper_pcap_open_live(&pcap_handle, iface->value, BUFSIZ, 0, 100, errbuf)) {
    log_add(NULL, ERROR, NAME, "Error al iniciar pcap");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }

  char bpf_filter[100];
  snprintf(bpf_filter, 100, "arp[6:2] = 2 and arp[14:4] = 0x%08x", htonl(dest_ip));
  struct bpf_program fp;
  if (!wrapper_pcap_compile(pcap_handle, &fp, bpf_filter)) {
    log_add(NULL, ERROR, NAME, "no se pudo compilar un filtro con pcap");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }
  if (!wrapper_pcap_setfilter(pcap_handle, &fp)) {
    log_add(NULL, ERROR, NAME, "Error al aplicar el filtro en pcap");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  signal(SIGINT, keyboard_interrupt_handle);

  struct pcap_pkthdr header;
  const u_char *packet = wrapper_pcap_next(pcap_handle, &header);
  if (!packet) {
    if (keyboard_interrupt) {
      log_clear(NULL);
      log_add(NULL, INFO, NAME, "Se dejo de escuchar paquetes...");
      log_show_and_clear(NULL);
      return EXIT_SUCCESS;
    }
    log_add(NULL, ERROR, NAME, "Error al capturar el paquete");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  signal(SIGINT, SIG_IGN);

  struct arp_header *arp = (struct arp_header *)(packet + sizeof(struct ethhdr));
  memcpy(result_mac, arp->sha, 6);

  char str_mac[18];
  if (!from_mac(str_mac, result_mac, 18)) {
    fprintf(stderr, "Error al convertir la direccion MAC desde el formato binario\n");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  (void)(free(out_prop_cfg->key), free(out_prop_cfg->value));
  prop_struct new_config = {strdup(out_prop_value), strdup(str_mac), MAC};
  int position = prop_reg_search_key(out_prop_value, *prop_register);
  if (position == -1) {
    log_add(NULL, ERROR, NAME, "No se encontro la propiedad de salida");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }
  if (!list_index_set(position, prop_register, &new_config, sizeof(prop_struct))) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se pudo modificar la propiedad de salida");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  pcap_close(pcap_handle);
  return EXIT_SUCCESS;
}

const Command cmd_resolve = {
    NAME,
    "Resuelve direccion MAC",
    "Resuelve la direccion MAC de un"
    " dispositivo en la red usando un ARP Resquest",
    "Comando: resolve\n"
    "Descripcion:\n"
    "  El comando `resolve` se utiliza para obtener la direccion MAC de un\n"
    "  dispositivo en la red a partir de su direccion IP. Utiliza un paquete\n"
    "  ARP request para descubrir la direccion MAC asociada.\n"
    "\n"
    "Uso:\n"
    "  resolve [propiedad de entrada (IP)] [propiedad de salida (MAC)]\n"
    "\n"
    "Argumentos especiales:\n"
    "  target   Utiliza las propiedades TARGET_ADDR como entrada\n"
    "           y TARGET_HWADDR como salida.\n"
    "  router   Utiliza las propiedades ROUTER_ADDR como entrada\n"
    "           y ROUTER_HWADDR como salida.\n"
    "\n"
    "Detalles:\n"
    "  - Obtiene la interfaz de red local definida por LOCAL_IFACE.\n"
    "  - Usa funciones auxiliares para obtener la IP y MAC local.\n"
    "  - Permite capturar respuestas ARP a traves de PCAP.\n"
    "  - Filtra solo respuestas ARP (opcode 2) provenientes de la IP de destino.\n"
    "  - Si se interrumpe con CTRL+C, se detiene la espera de paquetes.\n"
    "  - La direccion MAC obtenida se guarda en la propiedad de salida indicada.\n"
    "\n"
    "Ejemplos:\n"
    "  resolve target             # Usa TARGET_ADDR y guarda en TARGET_HWADDR\n"
    "  resolve router             # Usa ROUTER_ADDR y guarda en ROUTER_HWADDR\n"
    "  resolve VICTIMA_IP MAC_DESTINO  # Usa propiedades personalizadas\n"
    "\n"
    "Notas:\n"
    "  - Requiere permisos para capturar paquetes en la red.\n"
    "  - Se recomienda establecer previamente las propiedades\n"
    "    necesarias con el comando `set`.\n",
    {1, 2},
    fn_resolve,
};
