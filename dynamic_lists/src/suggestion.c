#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "list.h"
#include "logs.h"
#include "macros.h"
#include "suggestion.h"
#include "terminal.h"

SUGGEST_ptr suggest_new() {
  SUGGEST_ptr new_suggest = malloc(sizeof(SUGGEST));
  if (new_suggest == NULL) {
    log_add(NULL, ERROR, "Suggest", "No hay suficiente memoria para generar sugerencias");
    return NULL;
  }

  new_suggest->suggestions = list_new();
  if (new_suggest->suggestions == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "Suggest", "No se pudieron generar sugerencias");
    free(new_suggest);
    return NULL;
  }
  new_suggest->suggest_showed = 0;
  return new_suggest;
}

int suggest_add(SUGGEST_ptr sugg, char *sg_name, char *sg_value, char *sg_desc, suggest_types type) {
  if (sugg == NULL || !list_valid(sugg->suggestions)) { return 0; }
  suggest_struct temp_suggest;
  strncpy(temp_suggest.sg_name, sg_name, SUGGEST_NAME_LEN - 1);
  temp_suggest.sg_name[SUGGEST_NAME_LEN - 1] = '\0';
  strncpy(temp_suggest.sg_value, sg_value, SUGGEST_MAX_VALUE_SIZE - 1);
  temp_suggest.sg_value[SUGGEST_MAX_VALUE_SIZE - 1] = '\0';
  strncpy(temp_suggest.sg_desc, sg_desc, SUGGEST_DESC_LEN - 1);
  temp_suggest.sg_desc[SUGGEST_DESC_LEN - 1] = '\0';
  temp_suggest.sg_type = type;
  if (!list_push_back(sugg->suggestions, &temp_suggest, sizeof(suggest_struct))) return 0;
  return 1;
}

void suggest_show(SUGGEST_ptr sugg, int cursor_index, int position) {
  int results_size = list_size(*sugg->suggestions);
  if (results_size == 0) return;

  int padding = 0;
  for (int i = 0; i < results_size; i++) {
    NODE_ptr node = list_index_get(i, *sugg->suggestions);
    if (node) {
      suggest_struct *sg_value = (suggest_struct *)node->point;
      int val_len = strlen(sg_value->sg_name);
      if (val_len > padding) padding = val_len;
    }
  }

  term_size tz;
  if (!get_terminal_size(&tz)) return;
  for (int i = 0; i < results_size; i++) {
    NODE_ptr node = list_index_get(i, *sugg->suggestions);
    if (node) {
      suggest_struct *sg_value = (suggest_struct *)node->point;
      if ((i + 1) == position)
        printf("\r\n" BG_AMARILLO " %-*s " VERDE "%.*s " RESET, padding, sg_value->sg_name, tz.COLS - 5,
               sg_value->sg_desc);
      else
        printf("\r\n %-*s " VERDE "%.*s " RESET, padding, sg_value->sg_name, tz.COLS - 5, sg_value->sg_desc);
      sugg->suggest_showed++;
    }
  }

  printf("\033[%dA\033[%dG", sugg->suggest_showed, cursor_index);
  fflush(stdout);
}

void suggest_hide(SUGGEST_ptr sugg, int cursor_index) {
  for (int i = 0; i < sugg->suggest_showed; i++)
    printf("\n\033[2K");
  printf("\033[%dA\033[%dG", sugg->suggest_showed, cursor_index);
  fflush(stdout);
  sugg->suggest_showed = 0;
}

suggest_struct *suggest_get(SUGGEST_ptr sugg, int pos) {
  int results_size = list_size(*sugg->suggestions);
  if (results_size == 0) return NULL;
  for (int i = 0; i < results_size; i++) {
    if ((i + 1) == pos) {
      NODE_ptr node = list_index_get(i, *sugg->suggestions);
      if (node == NULL) return NULL;
      return (suggest_struct *)node->point;
    }
  }
  return NULL;
}

void suggest_clear(SUGGEST_ptr sugg) {
  if (list_size(*sugg->suggestions) > 0) {
    list_clear(sugg->suggestions);
    sugg->suggest_showed = 0;
  }
}

void suggest_free(SUGGEST_ptr sugg) {
  list_free(sugg->suggestions);
  free(sugg);
}
