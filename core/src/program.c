#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "history.h"
#include "instance.h"
#include "interpreter.h"
#include "list.h"
#include "logs.h"
#include "macros.h"
#include "program.h"
#include "read_string_utf8.h"
#include "string_utf8.h"
#include "suggestion.h"
#include "vm_def.h"
#include "xen_map.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"

#define NAME "shell"

int command_parser(char* cmd, ExecMode mode, SUGGEST_ptr* sugg, int sugg_pos) {
#define IF_SUGGEST if (mode == SUGGEST_MODE)
#define push_string(list, string)                                              \
  if (!list_push_back_string_node(list, string)) {                             \
    IF_SUGGEST {                                                               \
      suggest_free(*sugg);                                                     \
      *sugg = NULL;                                                            \
    }                                                                          \
    list_free(args);                                                           \
    return 0;                                                                  \
  }
  IF_SUGGEST suggest_clear(*sugg);
  char* comment_pos = strchr(cmd, '#');
  if (comment_pos != NULL) {
    IF_SUGGEST return 0;
    int comment_index = comment_pos - cmd;
    char* temp = strdup(cmd);
    memset(cmd, 0, CMDSIZ - 1);
    strncpy(cmd, temp, comment_index);
    cmd[comment_index] = '\0';
    free(temp);
  }
  IF_SUGGEST {
    if (sugg_pos < 0 || sugg_pos > (int)strlen(cmd)) {
      return 1;
    }

    int start = sugg_pos;
    while (start > 0 && cmd[start - 1] != ' ')
      start--;
    int end = sugg_pos;
    while (cmd[end] && cmd[end] != ' ')
      end++;

    int tok_len = end - start;
    if (tok_len <= 0) {
      return 1;
    }
    char* partial = strndup(cmd + start, tok_len);

    int is_first_token = (start == 0);

    if (is_first_token) {
      Xen_Instance* keys = Xen_Map_Keys(vm->root_context->ctx_instances);
      for (size_t i = 0; i < Xen_SIZE(keys); i++) {
        Xen_Instance* key = Xen_Operator_Eval_Pair_Steal2(
            keys, Xen_Number_From_ULong(i), Xen_OPR_GET_INDEX);
        if (Xen_TYPE(key) != &Xen_String_Implement) {
          Xen_DEL_REF(key);
          break;
        }
        if (strncmp(Xen_String_As_CString(key), partial, strlen(partial)) ==
            0) {
          char suggestion_cmd[CMDSIZ];
          snprintf(suggestion_cmd, sizeof(suggestion_cmd), "%.*s%s%s", start,
                   cmd, Xen_String_As_CString(key), cmd + end);
          suggest_add(*sugg, Xen_String_As_CString(key), suggestion_cmd,
                      "instance", COMMAND);
        }
        Xen_DEL_REF(key);
      }
      Xen_DEL_REF(keys);
    }
    free(partial);
    return 1;
  }
  LIST_ptr args = list_new();
  {
    int i = 0;
    while (cmd[i] == ' ') {
      i++;
    }

    if (i > 0) {
      int j = 0;
      while (cmd[i]) {
        cmd[j++] = cmd[i++];
      }
      cmd[j] = '\0';
    }
    int len = 0;
    while (cmd[len] != '\0') {
      len++;
    }

    while (len > 0 && cmd[len - 1] == ' ') {
      len--;
    }
    cmd[len] = '\0';
  }

  int args_size = list_size(*args);
  if (args_size == 0) {
    list_free(args);
    return 1;
  }
  NODE_ptr cmd_node = list_index_get(0, *args);
  if (cmd_node == NULL) {
    list_free(args);
    return 0;
  }
  list_free(args);
  return 1;
}

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
  if (!interpreter(file_content)) {
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
    LIST_ptr cmd = read_string_utf8();
    if (program.closed) {
      list_free(cmd);
      break;
    }
    char* cmd_str = string_utf8_get(cmd);
    list_free(cmd);
    if (!interpreter(cmd_str)) {
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
