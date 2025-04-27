#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "list.h"

typedef enum { IP = 0, MAC, IFACE, OTHER } config_types;

typedef struct {
  char *key;
  char *value;
  config_types type;
} config_struct;

typedef struct {
  config_types key;
  int (*func)(char *);
} types_struct;

LIST_ptr config_new();
int config_search_key(char *, LIST);
config_struct *config_value(char *, LIST);
int config_type_validate(config_types, char *);
void config_free(LIST_ptr);

extern LIST_ptr config;
extern const types_struct map_types[];

#endif
