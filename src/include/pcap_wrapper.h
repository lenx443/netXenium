#ifndef __PCAP_WRAPPER_H__
#define __PCAP_WRAPPER_H__

#include "pcap.h"
#include <pcap/pcap.h>

int wrapper_pcap_open_live(pcap_t **, char *, int, int, int, char *);
int wrapper_pcap_compile(pcap_t *, struct bpf_program *, const char *);
int wrapper_pcap_setfilter(pcap_t *, struct bpf_program *);
const u_char *wrapper_pcap_next(pcap_t *, struct pcap_pkthdr *);
int wrapper_pcap_loop(pcap_t *, int, pcap_handler, u_char *);

#endif
