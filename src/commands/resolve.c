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
#include "config.h"
#include "config_types.h"
#include "errs.h"
#include "functions.h"
#include "list.h"
#include "pcap_wrapper.h"
#include "program.h"

#define NAME "resolve"

static pcap_t *pcap_handle;
static uint8_t local_mac[6];
static uint8_t result_mac[6];

/*
 * Funcion que se ejecuta cuando
 * se preciona CTRL+C en el comando
 * `resolve`.
 */
static void keyboard_interrupt(int sign) { pcap_breakloop(pcap_handle); }

/*
 * El comando `resolve` se utiliza para
 * obtener la direccion MAC de un dispositivo
 * en la red empleando su direccion IP.
 * Este envia un ARP reply
 */
static int fn_resolve(LIST_ptr args) {
  // comprueba la valides de los argumentos
  if (list_size(*args) < 2) {
    fprintf(stderr,
            "Uso: " NAME
            " [propiedad de entrada (IP)] [propiedad de salida(MAC)]\n");
    return EXIT_FAILURE;
  }

  config_struct *iface = config_value("LOCAL_IFACE", *config);
  CHECK_ERROR_RETURN(iface == NULL, "Error al obtener la interfaz local",
                     EXIT_FAILURE);
  uint32_t local_ip;
  CHECK_ERROR_RETURN(!get_local_ip(&local_ip, iface->value),
                     "no se pudo obtener la direccion IP local", EXIT_FAILURE);

  CHECK_ERROR_RETURN(!get_local_mac(local_mac, iface->value),
                     "no se pudo obtener la direccion MAC local", EXIT_FAILURE);

  NODE_ptr in_prop = list_index_get(1, *args);
  CHECK_ERROR_RETURN(in_prop == NULL, "No se pudo acceder a [in prop]",
                     EXIT_FAILURE);

  char *in_prop_value = (char *)in_prop->point;
  char *out_prop_value = NULL;
  config_struct *out_prop_cfg = NULL;
  uint32_t dest_ip;

  if (strcmp(in_prop_value, "target") == 0) {
    /*
     * Si se usa el argumento `target`
     * se usa la propiedad `TARGET_ADDR`
     * como propiedad de entrada y la
     * propiedad `TARGET_HWADDR` como
     * propiedad de salida.
     */
    config_struct *target_prop_cfg = config_value("TARGET_ADDR", *config);
    out_prop_value = "TARGET_HWADDR";
    out_prop_cfg = config_value("TARGET_HWADDR", *config);
    to_addr(&dest_ip, target_prop_cfg->value);

  } else if (strcmp(in_prop_value, "router") == 0) {
    /*
     * Si se usa el argumento `router`
     * se usa la propiedad `ROUTER_ADDR`
     * como propiedad de entrada y la
     * propiedad `ROUTER_HWADDR` como
     * propiedad de salida.
     */
    config_struct *router_prop_cfg = config_value("ROUTER_ADDR", *config);
    out_prop_value = "ROUTER_HWADDR";
    out_prop_cfg = config_value("ROUTER_HWADDR", *config);
    to_addr(&dest_ip, router_prop_cfg->value);

  } else {
    /*
     * Si no se usan los anteriores algumentos
     * se usan como propiedad de entrada el
     * primer parametro y como propiedad de
     * salida el segundo parametro.
     */
    config_struct *in_prop_cfg = config_value(in_prop_value, *config);
    CHECK_ERROR_RETURN(in_prop_cfg == NULL,
                       "No se encontro la propiedad de entrada", EXIT_FAILURE);

    NODE_ptr out_prop = list_index_get(2, *args);
    CHECK_ERROR_RETURN(out_prop == NULL, "Error al obtener [out prop]",
                       EXIT_FAILURE);
    out_prop_value = (char *)out_prop->point;
    out_prop_cfg = config_value(out_prop_value, *config);
    CHECK_ERROR_RETURN(out_prop_cfg == NULL,
                       "No se encontro la propiedad de salida", EXIT_FAILURE);
    CHECK_ERROR_RETURN(!is_prop_ip(in_prop_cfg->type),
                       "La propiedad de entrada no es correcta", EXIT_FAILURE);
    CHECK_ERROR_RETURN(!is_prop_mac(out_prop_cfg->type),
                       "La propiedad de salida no es correcta", EXIT_FAILURE);

    CHECK_ERROR_RETURN(!to_addr(&dest_ip, in_prop_cfg->value),
                       "La IP de entrada es invalida", EXIT_FAILURE);
  }

  // se envia el ARP request
  CHECK_ERROR_RETURN(
      !send_arp_request(local_mac, local_ip, dest_ip, iface->value),
      "Error al enviar paquete ARP", EXIT_FAILURE);

  /*
   * Se usa PCAP para capturar paquetes
   * con un filtro que permite solo paquetes
   * ARP reply enviados por el objetivo.
   */
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_handle = NULL;
  CHECK_ERROR_RETURN(!wrapper_pcap_open_live(&pcap_handle, iface->value, BUFSIZ,
                                             0, 100, errbuf),
                     "Error al iniciar pcap", EXIT_FAILURE);

  char bpf_filter[100];
  snprintf(bpf_filter, 100, "arp[6:2] = 2 and arp[14:4] = 0x%08x",
           htonl(dest_ip));
  struct bpf_program fp;
  CHECK_ERROR_CODE_RETURN(!wrapper_pcap_compile(pcap_handle, &fp, bpf_filter),
                          "no se pudo compilar un filtro con pcap",
                          pcap_close(pcap_handle), EXIT_FAILURE);
  CHECK_ERROR_CODE_RETURN(!wrapper_pcap_setfilter(pcap_handle, &fp),
                          "Error al aplicar el filtro en pcap",
                          pcap_close(pcap_handle), EXIT_FAILURE);

  signal(SIGINT, keyboard_interrupt);

  struct pcap_pkthdr header;
  const u_char *packet = wrapper_pcap_next(pcap_handle, &header);
  CHECK_ERROR_CODE_RETURN(!packet, "Error al capturar el paquete",
                          pcap_close(pcap_handle), EXIT_FAILURE);

  signal(SIGINT, no_signal);

  struct arp_header *arp =
      (struct arp_header *)(packet + sizeof(struct ethhdr));
  memcpy(result_mac, arp->sha, 6);

  /*
   * Se convierte la MAC resultante
   * del formato binario a el formato
   * lejible y se pone como valor de
   * la propiedad de salida.
   */
  char str_mac[18];
  if (!from_hwaddr(str_mac, 18, result_mac)) {
    fprintf(stderr,
            "Error al convertir la direccion MAC desde el formato binario\n");
    pcap_close(pcap_handle);
    return EXIT_FAILURE;
  }

  (void)(free(out_prop_cfg->key), free(out_prop_cfg->value));
  config_struct new_config = {strdup(out_prop_value), strdup(str_mac), MAC};
  int position = config_search_key(out_prop_value, *config);
  CHECK_ERROR_CODE_RETURN(position == -1,
                          "No se encontro la propiedad de salida",
                          pcap_close(pcap_handle), EXIT_FAILURE);
  CHECK_ERROR_CODE_RETURN(
      !list_index_set(position, config, &new_config, sizeof(config_struct)),
      "No se pudo modificar la propiedad de salida", pcap_close(pcap_handle),
      EXIT_FAILURE);

  pcap_close(pcap_handle);
  return EXIT_SUCCESS;
}

// se define la estructura del comando
const Command cmd_resolve = {
    NAME,
    "resuelve la direccion MAC de un dispositivo en la red",
    fn_resolve,
};
