#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "config_types.h"
#include "error_codes.h"
#include "errs.h"
#include "list.h"

LIST_ptr config_new() {
  LIST_ptr cfg = list_new();
  memset(cfg, 0x00, sizeof(LIST));
  config_struct cfg_struct[] = {
      {strdup("LOCAL_IFACE"), strdup("lo"), IFACE},
      {strdup("ROUTER_ADDR"), strdup("0.0.0.0"), IP},
      {strdup("ROUTER_HWADDR"), strdup("00:00:00:00:00:00"), MAC},
      {strdup("TARGET_ADDR"), strdup("0.0.0.0"), IP},
      {strdup("TARGET_HWADDR"), strdup("00:00:00:00:00:00"), MAC},
      {"end", "end", OTHER}};
  for (int i = 0; strcmp(cfg_struct[i].key, "end") != 0; i++) {
    if (!list_push_back(cfg, &cfg_struct[i], sizeof(config_struct))) {
      list_free(cfg);
      return NULL;
    }
  }
  return cfg;
}

int config_search_key(char *key, LIST list) {
  NODE_ptr node = NULL;
  int n = 0;
  FOR_EACH(&node, list) {
    config_struct *current = (config_struct *)node->point;
    if (strcmp(current->key, key) == 0)
      return n;
    n++;
  }
  code_error = ERROR_LIST_FINDED;
  return -1;
}

config_struct *config_value(char *key, LIST list) {
  int n = config_search_key(key, list);
  if (n == -1)
    return NULL;
  NODE_ptr node = list_index_get(n, list);
  if (node == NULL)
    return NULL;
  config_struct *cfg = (config_struct *)node->point;
  return cfg;
}

int config_type_validate(config_types type, char *str) {
  int result_code = 0;
  if (type != OTHER) {
    for (int i = 0; map_types[i].key != OTHER; i++) {
      if (map_types[i].key == type) {
        result_code = map_types[i].func(str) ? 1 : 2;
      }
    }
  }
  if (!result_code)
    code_error = ERROR_LIST_FINDED;
  return result_code;
}

void config_free(LIST_ptr list) {
  if (list == NULL) {
    code_error = ERROR_LIST_NULL;
    return;
  }
  NODE_ptr current = list->head;
  while (current != NULL) {
    NODE_ptr next = current->next;
    config_struct *cfg = (config_struct *)current->point;
    free(cfg->key);
    free(cfg->value);
    free(current->point);
    free(current);
    current = next;
  }
  free(list);
}

LIST_ptr config;
const types_struct map_types[] = {
    {IP, is_addr}, {MAC, is_hwaddr}, {IFACE, is_iface}, {OTHER, NULL}};
