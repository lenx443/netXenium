#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <stdint.h>

#define color_reset "\x1b[0m"

char *color(int);
char *background(int);
char *mac_to_text(uint8_t *);
int get_local_ip(uint32_t *, char *);
int get_local_mac(uint8_t *, char *);
int get_mac(uint8_t *, uint8_t *, uint32_t, uint32_t, char *);
void send_arp(uint8_t *, uint32_t, uint8_t *, uint32_t, char *);

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
