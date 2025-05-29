#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "history.h"
#include "json_object.h"
#include "json_tokener.h"
#include "json_types.h"
#include "list.h"
#include "logs.h"
#include "macros.h"

HISTORY_ptr history_new(const char *filename) {
  HISTORY_ptr new_history = malloc(sizeof(HISTORY));
  if (new_history == NULL) {
    log_add(NULL, ERROR, "History", "No se pudo crear un nuevo historial");
    return NULL;
  }

  strncpy(new_history->filename, filename, sizeof(new_history->filename));
  new_history->filename[sizeof(new_history->filename) - 1] = '\0';

  new_history->cache_history = list_new();
  if (new_history->cache_history == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "History", "No se pudo crear el historial para el cache");
    free(new_history);
    return NULL;
  }

  new_history->local_history = list_new();
  if (new_history->local_history == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "History", "No se pudo crear el historial local");
    list_free(new_history->cache_history);
    free(new_history);
    return NULL;
  }

  if (access(filename, F_OK) != 0) {
    FILE *new_file = fopen(filename, "w");
    if (!new_file) {
      log_add(NULL, ERROR, "History", "No se pudo crear el archivo de historial");
      log_add_errno(NULL, ERROR, "History");
      list_free(new_history->local_history);
      list_free(new_history->cache_history);
      free(new_history);
      return NULL;
    }
    fclose(new_file);
  }

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    log_add(NULL, ERROR, "History", "No se pudo abrir el archivo de historial");
    log_add_errno(NULL, ERROR, "History");
    list_free(new_history->local_history);
    list_free(new_history->cache_history);
    free(new_history);
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  long pos = ftell(fp);
  int lines = 0;
  char ch;

  while (pos > 0 && lines <= HISMAXSIZ - 1) {
    fseek(fp, --pos, SEEK_SET);
    ch = fgetc(fp);
    if (ch == '\n') lines++;
  }

  if (pos > 0)
    fseek(fp, pos + 1, SEEK_SET);
  else
    fseek(fp, 0, SEEK_SET);

  char line[1024];
  while (fgets(line, 1024, fp)) {
    line[strcspn(line, "\n")] = '\0';
    HISTORY_struct history_line;

    struct json_object *parser = json_tokener_parse(line);

    struct json_object *time_stamp;
    if (!json_object_object_get_ex(parser, "time_stamp", &time_stamp)) {
      log_add(NULL, ERROR, "History", "Ah ocurrido un error al parciar el historial");
      json_object_put(parser);
      continue;
    }
    struct json_object *command;
    if (!json_object_object_get_ex(parser, "command", &command)) {
      log_add(NULL, ERROR, "History", "Ah ocurrido un error al parciar el historial");
      json_object_put(parser);
      continue;
    }

    history_line.time_stamp = json_object_get_int64(time_stamp);
    strncpy(history_line.command, json_object_get_string(command), sizeof(history_line.command));
    history_line.command[sizeof(history_line.command) - 1] = '\0';

    json_object_put(parser);

    if (!list_push_begin(new_history->cache_history, &history_line, sizeof(HISTORY_struct))) {
      DynSetLog(NULL);
      log_add(NULL, ERROR, "History", "No se pudo cargar el contenido del historial en memoria");
      break;
    }
  }
  fclose(fp);
  return new_history;
}

int history_push_line(HISTORY_ptr hist, HISTORY_struct line) {
  if (hist == NULL || !list_valid(hist->local_history)) {
    log_add(NULL, ERROR, "History", "No se pudo agregar un elemento al historial");
    return 0;
  }
  if (!list_push_begin(hist->local_history, &line, sizeof(HISTORY_struct))) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "History", "No se pudo agregar un elemento al historial");
    return 0;
  }
  return 1;
}

static HISTORY_struct *history_get_cache(HISTORY hist, int index) {
  int size = list_size(*hist.cache_history);
  if (size == 0) {
    log_add(NULL, INFO, "History", "El historial esta basio");
    return NULL;
  }
  if ((size - index) <= 0) {
    log_add(NULL, ERROR, "History", "El indice %d esta fuera de limite", index);
    return NULL;
  }

  NODE_ptr node_value = list_index_get(index, *hist.cache_history);
  if (node_value == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "History", "No se pudo obtener el elemento del historial");
    return NULL;
  }
  return (HISTORY_struct *)node_value->point;
}

HISTORY_struct *history_get(HISTORY hist, int index) {
  if (index < 0) {
    log_add(NULL, ERROR, "History", "El indice %d esta fuera de limite", index);
    return NULL;
  }

  if (list_empty(hist.local_history)) return history_get_cache(hist, index);
  int size = list_size(*hist.local_history);
  if ((size - index) <= 0) return history_get_cache(hist, index - size);

  NODE_ptr node_value = list_index_get(index, *hist.local_history);
  if (node_value == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, "History", "No se pudo obtener el elemento del historial");
    return NULL;
  }
  return (HISTORY_struct *)node_value->point;
}

int history_save(HISTORY hist) {
  FILE *fp = fopen(hist.filename, "a");
  if (!fp) {
    log_add(NULL, ERROR, "History", "No se pudo abrir el archivo de historial");
    log_add_errno(NULL, ERROR, "History");
    return 0;
  }
  NODE_ptr node = list_pop_back(hist.local_history);
  while (node != NULL) {
    HISTORY_struct *hist_struct = (HISTORY_struct *)node->point;
    fprintf(fp, "{\"time_stamp\": %ld, \"command\": \"%s\"}\n", hist_struct->time_stamp, hist_struct->command);
    node = list_pop_back(hist.local_history);
  }
  fclose(fp);
  return 1;
}

int history_size(HISTORY hist) { return list_size(*hist.cache_history) + list_size(*hist.local_history); }

void history_free(HISTORY_ptr hist) {
  list_free(hist->local_history);
  list_free(hist->cache_history);
  free(hist);
}
