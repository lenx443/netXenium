#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "compiler.h"
#include "history.h"
#include "interpreter.h"
#include "list.h"
#include "logs.h"
#include "macros.h"
#include "program.h"
#include "read_string_utf8.h"
#include "string_utf8.h"

void load_script(char* filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    log_add(NULL, ERROR, "Script-Loader", "No se pudo abrir el archivo: %s",
            filename);
    log_add_errno(NULL, ERROR, "Script-Loader");
    log_show_and_clear(NULL);
    return;
  }
  char line[CMDSIZ];
  LIST_ptr buffer = list_new();
  if (!buffer) {
    program.exit_code = EXIT_FAILURE;
    return;
  }
  while (fgets(line, CMDSIZ, fp)) {
    if (!string_utf8_push_back(buffer, line)) {
      list_free(buffer);
      fclose(fp);
      program.exit_code = EXIT_FAILURE;
      return;
    }
  }
  fclose(fp);
  char* file_content = string_utf8_get(buffer);
  list_free(buffer);
  if (!file_content) {
    program.exit_code = EXIT_FAILURE;
    return;
  }
  if (!interpreter(file_content, Xen_COMPILE_PROGRAM)) {
    log_show_and_clear(NULL);
  }
  free(file_content);
}

void shell_loop() {
  printf(AZUL "NetXenium" RESET " (C) " AMARILLO "Lenx443 2024-2025" RESET "\n"
              "Type " VERDE "help" RESET " for more info\n");
  const char* home = getenv("HOME");
  if (home == NULL) {
    printf("No se encontro la variable entorno HOME\n");
    return;
  };
  char history_path[1024];
  snprintf(history_path, 1024, "%s/.xenium_history", home);
  history = history_new(history_path);

  while (1) {
#ifndef SHELL_BASIC
    LIST_ptr cmd = read_string_utf8();
    if (program.closed) {
      list_free(cmd);
      break;
    }
    char* cmd_str = string_utf8_get(cmd);
    list_free(cmd);
#else
    fputs(" -> ", stdout);
    char* cmd_str = malloc(CMDSIZ);
    if (!fgets(cmd_str, CMDSIZ, stdin)) {
      fputs("\n", stdout);
      free(cmd_str);
      break;
    }
#endif
    if (!interpreter(cmd_str, Xen_COMPILE_REPL)) {
      log_show_and_clear(NULL);
      free(cmd_str);
      if (program.closed)
        break;
      continue;
    }
    free(cmd_str);
    if (program.closed)
      break;
  }
  history_save(*history);
  history_free(history);
}

Program_State program = {
    NULL, 0, NULL, 0, EXIT_SUCCESS, EXIT_SUCCESS,
};
HISTORY_ptr history = NULL;
