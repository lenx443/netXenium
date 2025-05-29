#include <pcap.h>
#include <pcap/pcap.h>
#include <sys/types.h>

#include "logs.h"
#include "pcap_wrapper.h"

int wrapper_pcap_open_live(pcap_t **handle, char *iface, int bufsiz, int mode, int time_r, char *errbuf) {
  *handle = pcap_open_live(iface, bufsiz, mode, time_r, errbuf);
  if (*handle == NULL) {
    log_add(NULL, ERROR, "PCAP", "Error al iniciar captura en vivo");
    log_add(NULL, ERROR, "PCAP", "Interfaz: %s, Tamaño de bufer: %d, Modo: %d", iface, bufsiz, mode);
    log_add(NULL, ERROR, "PCAP", "Pcap Log: %s", errbuf);
    return 0;
  }
  return 1;
}

int wrapper_pcap_compile(pcap_t *handle, struct bpf_program *fp, const char *filter) {
  if (pcap_compile(handle, fp, filter, 0, PCAP_NETMASK_UNKNOWN) < 0) {
    log_add(NULL, ERROR, "PCAP", "Error al compilar programa BPF");
    log_add(NULL, ERROR, "PCAP", "Pcap Log: %s", pcap_geterr(handle));
    return 0;
  }
  return 1;
}

int wrapper_pcap_setfilter(pcap_t *handle, struct bpf_program *fp) {
  if (pcap_setfilter(handle, fp) < 0) {
    log_add(NULL, ERROR, "PCAP", "Error al aplicar filtro BPF");
    log_add(NULL, ERROR, "PCAP", "Pcap Log: %s", pcap_geterr(handle));
    return 0;
  }
  return 1;
}

const u_char *wrapper_pcap_next(pcap_t *handle, struct pcap_pkthdr *hdr) {
  const u_char *pkg = pcap_next(handle, hdr);
  if (!pkg) {
    log_add(NULL, ERROR, "PCAP", "Error al capturar el proximo paquete");
    log_add(NULL, ERROR, "PCAP", "Pcap Log: %s", pcap_geterr(handle));
    return NULL;
  }
  return pkg;
}

int wrapper_pcap_loop(pcap_t *handle, int count, pcap_handler callback, u_char *user) {
  if (pcap_loop(handle, count, callback, user) < 0) {
    log_add(NULL, ERROR, "PCAP", "Error al capturar paquetes");
    log_add(NULL, ERROR, "PCAP", "Pcap Log: %s", pcap_geterr(handle));
    return 0;
  }
  return 1;
}
