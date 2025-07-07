#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "commands.h"
#include "history.h"
#include "interpreter.h"
#include "list.h"
#include "logs.h"
#include "macros.h"
#include "program.h"
#include "properties.h"
#include "read_string_utf8.h"
#include "string_utf8.h"
#include "suggestion.h"
#include "terminal.h"

#define NAME "shell"

static term_size current_term_size = {0};

static int is_special_char(char c) { return c == '\'' || c == '"' || c == '$'; }

static int is_escaped(const char *cmd, int pos) {
  int backslashes = 0;
  while (pos > 0 && cmd[--pos] == '\\')
    backslashes++;
  return (backslashes % 2) != 0;
}

static int extract_money_token(const char *cmd, int start, char money, char **out) {
  int i = start + 1;
  char prop_name[PROP_NAME_LEN];
  int j = 0;

  while (isalnum(cmd[i]) || cmd[i] == '_') {
    if (j < PROP_NAME_LEN - 1) prop_name[j++] = cmd[i];
    i++;
  }
  prop_name[j] = '\0';
  prop_struct *prop_value = prop_reg_value(prop_name, *prop_register);
  if (!prop_value) {
    log_add(NULL, ERROR, "Command Parser",
            "Error al obtener el valor de la propiedad " AZUL "%s" RESET, prop_name);
    log_show_and_clear(NULL);
    *out = malloc(1);
    (*out)[0] = '\0';
    return i;
  }

  int out_size = strlen(prop_value->value) + 1;
  *out = malloc(out_size);
  strncpy(*out, prop_value->value, out_size - 1);
  (*out)[out_size - 1] = '\0';
  return i;
};

static int extract_quoted_token(const char *cmd, int start, char quote, char **out) {
  int i = start + 1;
  char buffer[1024];
  int j = 0;

  while (cmd[i] != '\0') {
    if (cmd[i] == quote && !is_escaped(cmd, i)) {
      buffer[j] = '\0';
      *out = malloc(j + 1);
      strncpy(*out, buffer, j);
      (*out)[j] = '\0';

      return i + 1;
    }

    if (cmd[i] == '\\' && is_special_char(cmd[i + 1])) {
      buffer[j++] = cmd[i + 1];
      i += 2;
    } else if (cmd[i] == '$' && !is_escaped(cmd, i)) {
      char *money;
      int new_pos = extract_money_token(cmd, i, cmd[i], &money);
      for (int k = 0; money[k]; k++) {
        buffer[j++] = money[k];
      }
      free(money);
      i = new_pos;
    } else {
      buffer[j++] = cmd[i++];
    }
  }

  *out = NULL;
  return start + 1;
}

static int extract_normal_token(const char *cmd, int start, char **out) {
  int i = start;
  char buffer[1024];
  int j = 0;
  while (cmd[i] != '\0' && cmd[i] != ' ') {
    if (cmd[i] == '\\' && is_special_char(cmd[i + 1])) {
      buffer[j++] = cmd[i + 1];
      i += 2;
    } else if ((cmd[i] == '\'' || cmd[i] == '"') && !is_escaped(cmd, i)) {
      char *quoted;
      i = extract_quoted_token(cmd, i, cmd[i], &quoted);
      if (!quoted) {
        log_add(NULL, ERROR, "Command Parser", "Comilla sin cerrar detectada");
        log_add(NULL, ERROR, "Command Parser", "{%s} <- Exprecion invalida", cmd);
        log_show_and_clear(NULL);
        free(*out);
        *out = NULL;
        program.closed = 1;
        return strlen(cmd);
      }
      for (int k = 0; quoted[k]; k++)
        buffer[j++] = quoted[k];
      free(quoted);
    } else if (cmd[i] == '$' && !is_escaped(cmd, i)) {
      char *money;
      i = extract_money_token(cmd, i, cmd[i], &money);
      for (int k = 0; money[k]; k++)
        buffer[j++] = money[k];
      free(money);
    } else {
      buffer[j++] = cmd[i++];
    }
  }
  buffer[j] = '\0';
  *out = strdup(buffer);
  return i;
}

int command_parser(char *cmd, ExecMode mode, SUGGEST_ptr *sugg, int sugg_pos) {
#define IF_SUGGEST if (mode == SUGGEST_MODE)
#define IF_EXEC if (mode == EXEC_MODE)
#define push_string(list, string)                                                        \
  if (!list_push_back_string_node(list, string)) {                                       \
    IF_SUGGEST {                                                                         \
      suggest_free(*sugg);                                                               \
      *sugg = NULL;                                                                      \
    }                                                                                    \
    list_free(args);                                                                     \
    return 0;                                                                            \
  }
  IF_SUGGEST suggest_clear(*sugg);
  char *comment_pos = strchr(cmd, '#');
  if (comment_pos != NULL) {
    IF_SUGGEST return 0;
    int comment_index = comment_pos - cmd;
    char *temp = strdup(cmd);
    memset(cmd, 0, CMDSIZ - 1);
    strncpy(cmd, temp, comment_index);
    cmd[comment_index] = '\0';
    free(temp);
  }
  IF_SUGGEST {
    if (sugg_pos < 0 || sugg_pos > (int)strlen(cmd)) { return 1; }

    int start = sugg_pos;
    while (start > 0 && cmd[start - 1] != ' ')
      start--;
    int end = sugg_pos;
    while (cmd[end] && cmd[end] != ' ')
      end++;

    int tok_len = end - start;
    if (tok_len <= 0) { return 1; }
    char *partial = strndup(cmd + start, tok_len);

    int is_first_token = (start == 0);

    char *money = strchr(partial, '$');
    if (money != NULL) {
      int money_pos = money - partial;
      NODE_ptr node = NULL;
      FOR_EACH(&node, *prop_register) {
        prop_struct *reg_val = (prop_struct *)node->point;
        if (strncmp(reg_val->key, money + 1, strlen(money) - 1) == 0) {
          char suggestion_cmd[CMDSIZ];
          snprintf(suggestion_cmd, sizeof(suggestion_cmd), "%.*s%.*s$%s%s", start, cmd,
                   money_pos, partial, reg_val->key, cmd + end);
          char *type = strdup("");
          for (int i = 0; map_types[i].key != OTHER; i++) {
            if (map_types[i].key == reg_val->type) {
              free(type);
              type = strdup(map_types[i].key_str);
            }
          }
          suggest_add(*sugg, reg_val->key, suggestion_cmd, type, COMMAND);
          free(type);
        }
      }
    } else if (is_first_token) {
      for (int i = 0; cmds_table[i] != NULL; i++) {
        if (strncmp(cmds_table[i]->name, partial, strlen(partial)) == 0) {
          char suggestion_cmd[CMDSIZ];
          snprintf(suggestion_cmd, sizeof(suggestion_cmd), "%.*s%s%s", start, cmd,
                   cmds_table[i]->name, cmd + end);
          suggest_add(*sugg, cmds_table[i]->name, suggestion_cmd,
                      cmds_table[i]->short_desc, COMMAND);
        }
      }
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
  int cmd_offset = 0;

  IF_EXEC while (cmd[cmd_offset] != '\0') {
    while (cmd[cmd_offset] == ' ')
      cmd_offset++;

    if (cmd[cmd_offset] == '\0') break;

    char *token = NULL;
    cmd_offset = extract_normal_token(cmd, cmd_offset, &token);
    if (token && token[0] != '\0') { push_string(args, token); }
    free(token);
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
  char *cmd_str = (char *)cmd_node->point;
  int matched = 0;
  for (int i = 0; cmds_table[i] != NULL; i++) {
    if (strcmp(cmds_table[i]->name, cmd_str) == 0) {
      IF_EXEC {
        if ((args_size - 1) < cmds_table[i]->args_len[0] ||
            (args_size - 1) > cmds_table[i]->args_len[1]) {
          log_add(NULL, ERROR, "Command Parser", "\"{%s}\" <- Exprecion incorrecta", cmd);
          log_show_and_clear(NULL);
          program.return_code = 153;
          free(args);
          return 0;
        }
        program.return_code = cmds_table[i]->func(args);
        if (program.return_code == 153) {
          log_add(NULL, ERROR, "Command Parser", "\"{%s}\" <- Exprecion incorrecta", cmd);
          log_show_and_clear(NULL);
          free(args);
          return 0;
        }
      }
      matched = 1;
      break;
    }
  }
  IF_EXEC if (!matched) {
    log_add(NULL, ERROR, "Command Parser",
            "\"|%d>%s\" <- " AMARILLO "\'%s\'" RESET " no encontrado", strlen(cmd_str),
            cmd, cmd_str);

    log_show(NULL);
    log_clear(NULL);
    program.return_code = EXIT_FAILURE;
  }

  list_free(args);
  IF_EXEC return matched;
  return 1;
}

void load_script(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    log_add(NULL, ERROR, "Script-Loader", "No se pudo abrir el archivo: %s", filename);
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
  char *file_content = string_utf8_get(buffer);
  if (!file_content) {
    list_free(buffer);
    program.exit_code = EXIT_FAILURE;
    return;
  }
  if (!interpreter(file_content)) { log_show_and_clear(NULL); }
  free(file_content);
  list_free(buffer);
}

void shell_loop(char *name) {
  printf(AZUL "NetXenium" RESET " (C) " AMARILLO "Lenx443 2024-2025" RESET "\n"
              "Type " VERDE "help" RESET " for more info\n");
  const char *home = getenv("HOME");
  if (home == NULL) {
    printf("No se encontro la variable entorno HOME\n");
    return;
  };
  char history_path[1024];
  snprintf(history_path, 1024, "%s/.xenium_history", home);
  history = history_new(history_path);

  while (1) {
    LIST_ptr cmd = read_string_utf8();
    char *cmd_str = string_utf8_get(cmd);
    if (!interpreter(cmd_str)) {
      log_show_and_clear(NULL);
      free(cmd_str);
      break;
    }
    free(cmd_str);
    if (program.closed) break;
  }
  history_save(*history);
  history_free(history);
}

Program_State program = {
    NULL, 0, NULL, 0, EXIT_SUCCESS, EXIT_SUCCESS,
};
const Command *cmds_table[] = {
    &cmd_help,      &cmd_exit,  &cmd_new,   &cmd_del,           &cmd_get,
    &cmd_set,       &cmd_echo,  &cmd_input, &cmd_clear_history, &cmd_resolve,
    &cmd_arp_spoof, &cmd_iface, NULL,
};
HISTORY_ptr history = NULL;
