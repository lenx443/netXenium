#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "list.h"
#include "logs.h"
#include "properties.h"
#include "properties_types.h"

LIST_ptr prop_reg_new() {
  LIST_ptr new_prop = list_new();
  return new_prop;
}

int prop_reg_add(LIST_ptr list, char *key, char *value, prop_types type) {
  if (!list_valid(list)) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "Properties-Add", "La lista es invalida");
    return 0;
  }
  prop_struct new_prop = {
      strdup(key),
      strdup(value),
      type,
  };
  if (!list_push_begin(list, &new_prop, sizeof(prop_struct))) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "Properties-Add", "No se pudo almacenar la propiedad");
    return 0;
  }
  return 1;
}

int prop_reg_search_key(const char *key, LIST list) {
  NODE_ptr node = NULL;
  int n = 0;
  FOR_EACH(&node, list) {
    prop_struct *current = (prop_struct *)node->point;
    if (strcmp(current->key, key) == 0) return n;
    n++;
  }
  log_add(NULL, ERROR, "Properties-Search-Key", "El elemento no fue encontrado");
  log_add(NULL, ERROR, "Properties-Search-Key", "key: " AMARILLO "{%s}" RESET, key);
  return -1;
}

prop_struct *prop_reg_value(const char *key, LIST list) {
  int n = prop_reg_search_key(key, list);
  if (n == -1) return NULL;
  NODE_ptr node = list_index_get(n, list);
  if (node == NULL) {
    DynSetLog(NULL);
    return NULL;
  }
  prop_struct *new_prop = (prop_struct *)node->point;
  return new_prop;
}

int prop_reg_type_validate(prop_types type, char *key) {
  int result_code = 0;
  if (type != OTHER) {
    for (int i = 0; map_types[i].key != OTHER; i++) {
      if (map_types[i].key == type) {
        if (map_types[i].validate(key)) {
          result_code = map_types[i].validate(key) ? 1 : 2;
        } else
          result_code = 1;
      }
    }
  }
  if (result_code == 0) {
    log_add(NULL, ERROR, "Properties-Type-Validate", "El elemento no fue encontrado");
    log_add(NULL, ERROR, "Properties", "key: " AMARILLO "{%s}" RESET, key);
  }
  return result_code;
}

void prop_reg_free(LIST_ptr list) {
  if (!list_valid(list)) return;
  NODE_ptr current = list->head;
  while (current != NULL) {
    NODE_ptr next = current->next;
    prop_struct *new_prop = (prop_struct *)current->point;
    free(new_prop->key);
    free(new_prop->value);
    free(current->point);
    free(current);
    current = next;
  }
  free(list);
}

LIST_ptr prop_register;
const types_struct map_types[] = {
    {
        IP,
        "IP",
        is_ip,
        to_ip,
        from_ip,
    },
    {
        MAC,
        "MAC",
        is_mac,
        to_mac,
        from_mac,
    },
    {
        IFACE,
        "IFACE",
        is_iface,
        NULL,
        NULL,
    },
    {
        STRING,
        "STRING",
        is_string,
        to_string,
        from_string,
    },
    {
        OTHER,
        "OTHER",
        NULL,
        NULL,
        NULL,
    },
};
