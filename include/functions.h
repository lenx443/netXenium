#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <stdint.h>

#define color_reset "\x1b[0m"

char * color(int index);
char * background(int index);
char * mac_to_text(uint8_t * mac);
uint32_t get_local_ip(char * iface);
uint8_t * get_local_mac(char * iface);
uint8_t * get_mac(uint8_t * source_mac, uint32_t source_ip, uint32_t dest_ip, char * iface);
void send_arp(uint8_t * source_mac, uint32_t source_ip, uint8_t * dest_mac, uint32_t dest_ip, char * iface);

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
