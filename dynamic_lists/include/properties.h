#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "list.h"

typedef enum {
  IP = 0,
  MAC,
  IFACE,
  STRING,
  OTHER,
} prop_types;

typedef struct {
  char *key;
  char *value;
  prop_types type;
} prop_struct;

typedef struct {
  prop_types key;
  char *key_str;
  int (*validate)(char *);
  int (*to_type)(void *, char *);
  int (*from_type)(char *, void *, size_t);
} types_struct;

LIST_ptr prop_reg_new();
int prop_reg_add(LIST_ptr, char *, char *, prop_types);
int prop_reg_search_key(char *, LIST);
prop_struct *prop_reg_value(char *, LIST);
int prop_reg_type_validate(prop_types, char *);
void prop_reg_free(LIST_ptr);

extern LIST_ptr prop_register;
extern const types_struct map_types[];

#endif
