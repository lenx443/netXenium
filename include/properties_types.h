#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#include "properties.h"
#include <stddef.h>
#include <stdint.h>

int is_ip(char *);
int to_ip(void *, char *);
int from_ip(char *, void *, size_t);

int is_mac(char *);
int to_mac(void *, char *);
int from_mac(char *, void *, size_t);

int is_iface(char *);

int is_string(char *);
int to_string(void *, char *);
int from_string(char *, void *, size_t);

int is_prop_ip(prop_types);
int is_prop_mac(prop_types);
int is_prop_iface(prop_types);

#endif
