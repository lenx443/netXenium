#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#include "config.h"
#include <stddef.h>
#include <stdint.h>

int is_iface(char *);

int is_addr(char *);
int to_addr(uint32_t *, char *);
int from_addr(char *, uint32_t);

int is_hwaddr(char *);
int to_hwaddr(uint8_t *, char *);
int from_hwaddr(char *, size_t, uint8_t *);

int is_prop_ip(config_types);
int is_prop_mac(config_types);
int is_prop_iface(config_types);

#endif
