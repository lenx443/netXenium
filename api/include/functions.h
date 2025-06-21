#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <stdint.h>

#include "list.h"

int strip_ansi_escape_strlen(const char *);
void strip_ansi_escape(char *str);
int get_local_ip(uint32_t *, char *);
int get_local_netmask(uint32_t *, char *);
int get_local_broadcast(uint32_t *, char *);
int get_local_mac(uint8_t *, char *);
int set_local_ip(uint32_t, char *);
int set_local_netmask(uint32_t, char *);
int set_local_broadcast(uint32_t, char *);
int set_local_mac(uint8_t *, char *);
LIST_ptr get_ifaces_list();
int send_arp_request(uint8_t *, uint32_t, uint32_t, char *);
int send_arp_reply(uint8_t *, uint32_t, uint8_t *, uint32_t, char *);

struct arp_header {
  uint16_t hardware_type;
  uint16_t protocol_type;
  uint8_t hardware_len;
  uint8_t protocol_len;
  uint16_t op;
  uint8_t sha[6];
  uint32_t spa;
  uint8_t tha[6];
  uint32_t tpa;
} __attribute__((packed));

#endif
