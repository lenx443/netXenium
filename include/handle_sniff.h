#ifndef __HANDLE_SNIFF_H__
#define __HANDLE_SNIFF_H__

#include <pcap.h>
#include <stdint.h>

void callback(u_char *, const struct pcap_pkthdr *, const u_char *);
void wait_handle(const char *, uint32_t, uint8_t *, uint32_t, uint8_t *);

#endif
